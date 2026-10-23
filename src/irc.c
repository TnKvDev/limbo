#include "irc.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* The destination server adds ":nick!user@host PRIVMSG #chan " in front of what we
 * send before it reaches the other clients, and the whole line must stay below 512
 * bytes. So we cut our part well before 510 bytes. */
#define FWD_MAX_LEN 400

int register_irc(struct srv_info* srv) {
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "NICK %s\r\n", BOT_NICK);
    if (tls_send(srv->ssl, cmd) == ERROR) {
        log(ERROR, "Failed to send nickname");
        return ERROR;
    }

    snprintf(cmd, sizeof(cmd), "USER %s 0 * :Limbo Bot\r\n", BOT_NICK);
    if (tls_send(srv->ssl, cmd) == ERROR) {
        log(ERROR, "Failed to send username");
        return ERROR;
    }

    // Wait for 001 (RPL_WELCOME); the server may PING us before that.
    bool welcomed = false;
    char line[BUF_SIZE];
    while (!welcomed) {
        int rc = irc_fill(srv);
        if (rc != TLS_OK && rc != TLS_AGAIN) {
            log(ERROR, "Connection to %s lost during registration", srv->conf.hostname);
            return ERROR;
        }

        // Stop popping lines once welcomed; leftovers stay buffered for the main loop.
        while (!welcomed && irc_pop_line(srv, line, sizeof(line))) {
            struct irc_msg msg;
            if (parse_irc_line(line, &msg) == ERROR)
                continue;

            if (strcasecmp(msg.command, "PING") == 0) {
                irc_handle_ping_cmd(srv->ssl, &msg);
            } else if (strcmp(msg.command, "001") == 0) {
                welcomed = true;
            } else if (atoi(msg.command) == ERR_NICKNAMEINUSE) {
                log(ERROR, "Nickname %s is already in use on %s", BOT_NICK, srv->conf.hostname);
                return ERROR;
            }
        }
    }

    snprintf(cmd, sizeof(cmd), "JOIN %s\r\n", srv->conf.channel);
    if (tls_send(srv->ssl, cmd) == ERROR) {
        log(ERROR, "Failed to join %s", srv->conf.channel);
        return ERROR;
    }
    log(DEBUG, "Joined Channel %s on %s", srv->conf.channel, srv->conf.hostname);

    return 0;
}

/* message = [ "@" tags SPACE ] [ ":" prefix SPACE ] command [ SPACE params ]
 * Fields are consumed strictly left to right, so a nick or message text that
 * happens to contain "PING" or "PRIVMSG" can never be mistaken for the command. */
int parse_irc_line(char* line, struct irc_msg* msg) {
    char* p = line;
    msg->prefix = NULL;

    // IRCv3 message tags (we don't request them, but skip them if they show up)
    if (*p == '@') {
        p = strchr(p, ' ');
        if (p == NULL)
            return ERROR;
        while (*p == ' ') p++;
    }

    if (*p == ':') {
        msg->prefix = p + 1;
        p = strchr(p, ' ');
        if (p == NULL)
            return ERROR;
        *p++ = '\0';
        while (*p == ' ') p++;
    }

    if (*p == '\0')
        return ERROR;

    msg->command = p;
    p = strchr(p, ' ');
    if (p == NULL) {
        msg->params = msg->command + strlen(msg->command); // empty string
    } else {
        *p++ = '\0';
        while (*p == ' ') p++;
        msg->params = p;
    }

    return 0;
}

msg_type classify_msg(const struct irc_msg* msg) {
    // whole-token comparison: "PRIVMSGX" or "PINGPONG" don't match
    if (strcasecmp(msg->command, "PING") == 0)
        return PING;
    if (strcasecmp(msg->command, "PRIVMSG") == 0)
        return PRIVMSG;
    return NOTHING_INTERESTING;
}

int irc_handle_ping_cmd(SSL* ssl, const struct irc_msg* msg) {
    // Echo the token back unchanged: "PING :token" -> "PONG :token"
    const char* token = msg->params;
    if (*token == ':')
        token++;

    char response[BUF_SIZE];
    snprintf(response, sizeof(response), "PONG :%s\r\n", token);
    if (tls_send(ssl, response) == ERROR) {
        log(ERROR, "Failed to send ping response");
        return ERROR;
    }
    log(DEBUG, "Sent PONG");
    return 0;
}

/* Forwards a channel message from src's channel to dst's channel as "<nick> text". */
int irc_handle_privmsg_cmd(struct srv_info* src, struct srv_info* dst, const struct irc_msg* msg) {
    // Sender nick = prefix up to '!' (or '@' / end of string)
    if (msg->prefix == NULL)
        return 0;

    char nick[MAX_NICK_LENGTH];
    size_t nick_len = strcspn(msg->prefix, "!@");
    if (nick_len == 0 || nick_len >= sizeof(nick))
        return 0;
    memcpy(nick, msg->prefix, nick_len);
    nick[nick_len] = '\0';

    // Never forward ourselves
    if (strcasecmp(nick, BOT_NICK) == 0)
        return 0;

    // First param is the target. Only forward what was said in the bridged channel
    // (private messages to the bot and other channels are ignored).
    const char* target = msg->params;
    const char* space = strchr(target, ' ');
    if (space == NULL)
        return 0;

    size_t target_len = (size_t)(space - target);
    if (target_len != strlen(src->conf.channel) ||
        strncasecmp(target, src->conf.channel, target_len) != 0)
        return 0;

    // Everything after the target is the text; a leading ':' marks the trailing param.
    const char* text = space + 1;
    while (*text == ' ') text++;
    if (*text == ':')
        text++;

    // CTCP: only /me (ACTION) is bridged, everything else (VERSION, PING, ...) is dropped
    char body[BUF_SIZE];
    if (text[0] == '\x01') {
        if (strncmp(text + 1, "ACTION ", 7) != 0)
            return 0;
        snprintf(body, sizeof(body), "* %s %s", nick, text + 8);
        size_t blen = strlen(body);
        if (blen > 0 && body[blen - 1] == '\x01')
            body[blen - 1] = '\0';
    } else {
        snprintf(body, sizeof(body), "<%s> %s", nick, text);
    }

    // Build "PRIVMSG #dst :<nick> text", cut it to FWD_MAX_LEN and append CRLF
    char out[BUF_SIZE + 128];
    int n = snprintf(out, sizeof(out), "PRIVMSG %s :%s", dst->conf.channel, body);
    if (n < 0)
        return ERROR;

    size_t len = strlen(out);
    if (len > FWD_MAX_LEN) {
        len = FWD_MAX_LEN;
        // Don't cut a UTF-8 character in half: if the first dropped byte is a
        // continuation byte (10xxxxxx) we are in the middle of a character, so step back.
        while (len > 0 && ((unsigned char)out[len] & 0xC0) == 0x80)
            len--;
    }
    out[len] = '\r';
    out[len + 1] = '\n';
    out[len + 2] = '\0';

    log(DEBUG, "Forwarding %s -> %s: %s", src->conf.hostname, dst->conf.hostname, out);
    return tls_send(dst->ssl, out);
}
