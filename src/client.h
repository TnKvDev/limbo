#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
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
    char buf[BUF_SIZE];
    struct config conf;
    struct sockaddr_in srv;
    struct hostent* srvent;
    SSL* ssl;
    int sockfd;
    char connection_established; // Boolean
};

int connect_to(const char* hostname, int port, struct srv_info* srv, SSL_CTX* ctx);
void handle_data(char* buf, size_t bytes, struct config src_conf, struct config dst_conf, SSL* ssl);
int reconnect(struct srv_info* srv);
void close_connection(struct srv_info srv);

#endif // CLIENT_H
