#include "irc.h"
#include "logger.h"

#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096
const char* nick_cmd = "NICK limbo\r\n";
const char* user_cmd = "USER limbo 0 * :Limbo Bot\r\n";

// TODO find out when you can actually join a channel.
int register_irc(struct srv_info srv, struct config conf) {
    if (tls_send(srv.ssl, nick_cmd) == ERROR) {
        log(ERROR, "Failed to send nickname");
        return ERROR;
    }

    if (tls_send(srv.ssl, user_cmd) == ERROR) {
        log(ERROR, "Failed to send username");
        return ERROR;
    }

    bool welcomed = 0x0000; // Boolean
    char buf[BUF_SIZE];
    size_t bytes;
    struct cmd_break_up* brkup = malloc(sizeof(struct cmd_break_up));
    while (!welcomed) {
        if (tls_read(srv.ssl, buf, BUF_SIZE, &bytes) == TLS_OK) {
            brkup = break_cmd_up(buf, bytes);
            if (strcmp(brkup->hostname, "PING ") == 0) {
                int stat = handle_ping_cmds(srv.ssl, buf, bytes);
            } else if (brkup->numeric == 1) {
                welcomed = true;
            }
        }
    }
    
    char channel[64];
    snprintf(channel, sizeof(channel), "JOIN %s\r\n", conf.channel);
    if (tls_send(srv.ssl, channel) == ERROR) {
        log(ERROR, "Failed to join %s", conf.channel);
        return ERROR;
    }
    log(DEBUG, "Joined Channel %s", conf.channel);

    free(brkup);
    return 0;
}

struct cmd_break_up* break_cmd_up(char* buf, size_t bytes) {
    buf[bytes] = '\0';
    struct cmd_break_up* brkup = malloc(sizeof(struct cmd_break_up));
    int index = 0;

    // Ping has other syntax than any other IRC command
    if (strncmp(buf, "PING ", 5) == 0) {
        strcpy(brkup->hostname, "PING ");
        return brkup;
    }

    // get hostname
    char* pos = strchr(buf, ' ');
    if (pos == NULL) {
        log(DEBUG, "strchr() failed");
        return NULL;
    }
    
    *pos = '\0';
    strncpy(brkup->hostname, &buf[1], (pos - &buf[0]));
    log(DEBUG, "BRKUP HOSTNAME: %s", brkup->hostname);
    *pos = ' ';
    
    // get cmd numeral
    pos++;
    char numeral_char[4] = {'\0'};
    strncpy(numeral_char, pos, 3);
    log(DEBUG, "BRKUP NUMERIC: %s", numeral_char);
    brkup->numeric = atoi(numeral_char);

    // replace null terminator
    pos--;
    *pos = ' ';

    return brkup;
}

int handle_ping_cmds(SSL* ssl, char* buf, size_t bytes) {
    char response[64];
    snprintf(response, sizeof(response), "PONG %s\r\n", &buf[5]);
    if (tls_send(ssl, response) == ERROR) {
        log(ERROR, "Failed to send ping response");
        return ERROR;
    }
    log(DEBUG, "Sent bytes: %s", response);
    return 0;
}
