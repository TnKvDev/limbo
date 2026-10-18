#ifndef IRC_H
#define IRC_H

#include "client.h"
#include "config.h"
#include "tls.h"

/* from urircd numeric.h */
#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404

#define ERR_FORBIDDENCHANNEL 448

#define ERR_NOTREGISTERED    451
#define ERR_ALREADYREGISTRED 462

struct cmd_break_up {
    char hostname[MAX_HOSTNAME_LENGTH];
    char target[MAX_NICK_LENGTH];
    int numeric;
};

int register_irc(struct srv_info srv, struct config conf);

struct cmd_break_up* break_cmd_up(char* buf, size_t bytes);

int handle_ping_cmds(SSL* ssl, char* buf, size_t bytes);

#endif // IRC_H
