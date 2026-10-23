#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <openssl/ssl.h>

#include "config.h"

#define BUF_SIZE 4096

#define MAX_NICK_LENGTH 32
#define MAX_MSG_LENGTH 512
#define MAX_HOSTNAME_LENGTH 63

struct srv_info {
    char buf[BUF_SIZE];          // raw bytes
    size_t buflen;               // number of "valid" bytes in buf
    struct config conf;
    struct sockaddr_in srv;
    struct hostent* srvent;
    SSL* ssl;
    int sockfd;
    char connection_established; // Boolean
};

typedef enum msg_type {
    PING,
    PRIVMSG,
    NOTHING_INTERESTING
} msg_type;

int connect_to(const char* hostname, int port, struct srv_info* srv, SSL_CTX* ctx);

/* @brief Reads from the TLS connection and appends to srv->buf. */
int irc_fill(struct srv_info* srv);

int irc_pop_line(struct srv_info* srv, char* line, size_t line_size);

/* @brief Splits everything buffered in src into lines and handles them */
void handle_data(struct srv_info* src, struct srv_info* dst);

/* @brief Reads whatever src has available. Returns 0 or ERROR if the connection is gone. */
int service_connection(struct srv_info* src, struct srv_info* dst);

/* Closes and re-establishes the connection (retries until it works) */
int reconnect(struct srv_info* srv, SSL_CTX* ctx);
void close_connection(struct srv_info srv);

#endif // CLIENT_H
