#ifndef IRC_H
#define IRC_H

#include "client.h"
#include "config.h"
#include "tls.h"

#define BOT_NICK "limbo"

/* from urircd numeric.h */
#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_NICKNAMEINUSE    433

#define ERR_FORBIDDENCHANNEL 448

#define ERR_NOTREGISTERED    451
#define ERR_ALREADYREGISTRED 462

struct irc_msg {
    char* prefix;   // "nick!user@host" or server name. NULL if the line has none
    char* command;  // "PRIVMSG", "PING", ...
    char* params;   // everything after the command
};

int register_irc(struct srv_info* srv);

int parse_irc_line(char* line, struct irc_msg* msg);
msg_type classify_msg(const struct irc_msg* msg);

int irc_handle_ping_cmd(SSL* ssl, const struct irc_msg* msg);
int irc_handle_privmsg_cmd(struct srv_info* src, struct srv_info* dst, const struct irc_msg* msg);

#endif // IRC_H
