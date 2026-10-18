#include "client.h"
#include "tls.h"
#include "logger.h"

int connect_to(const char* hostname, int port, struct srv_info* srv, SSL_CTX* ctx) {
    srv->connection_established = false;
    log(DEBUG, "Building connection to %s", hostname);

    // Create socket
    srv->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->sockfd == -1) {
        log(ERROR, "Failed to create a socket for %s", hostname);
        goto error;
    }
    log(DEBUG, "Created socket file descriptor for %s", hostname);

    // Fetch information from the host
    srv->srvent = gethostbyname(hostname);
    if(srv->srvent == NULL) {
        log(ERROR, "Failed to fetch host information from %s", hostname);
        goto error;
    }
    log(DEBUG, "Fetched information about %s", hostname);

    memset(srv, 0, sizeof(struct sockaddr_in)); // Fill with zeros
    memcpy(&(srv->srv.sin_addr.s_addr), srv->srvent->h_addr, srv->srvent->h_length); // IP
    srv->srv.sin_family = AF_INET; // IPv4
    srv->srv.sin_port = htons(port); // Port

    // Create SSL object
    srv->ssl = SSL_new(ctx);
    if (srv->ssl == NULL) {
        log(ERROR, "Failed to create SSL object for %s", hostname);
        goto error;
    }
    log(DEBUG, "Created SSL object for %s", hostname);

    // Server name indicator
    if (!SSL_set_tlsext_host_name(srv->ssl, hostname)) {
        log(ERROR, "Failed to set hostname");
        goto error;
    }
    log(DEBUG, "Set SNI to %s", hostname);

    if (!SSL_set1_host(srv->ssl, hostname)) {
        log(ERROR, "Failed to set certificate verification hostname");
        goto error;
    }
    log(DEBUG, "Set host to %s", hostname);

    SSL_set_fd(srv->ssl, srv->sockfd);
    log(DEBUG, "Set sockfd");

    // CONNECTION
    if (connect(srv->sockfd, (struct sockaddr*)&(srv->srv), sizeof(srv->srv))) {
        log(ERROR, "Failed to connect to %s", hostname);
        goto error;
    }
    
    if (perform_tls_handshake(srv->ssl) == ERROR) {
        log(ERROR, "Failed to perform handshake");
        goto error;
    }
    
 success:
    srv->connection_established = true;
    return 0;
    
 error:
    close(srv->sockfd);
    return ERROR;
}

char* getusr(char* buf) {
    char* usr = malloc(MAX_NICK_LENGTH);
    char* i = strchr(buf, '!');
    // *i = ' ';
    
    *i = '\0';
    snprintf(usr, sizeof(usr), "%s", &buf[1]);
    log(DEBUG, "USER: %s", usr);
    *i = '!';
    
    return usr;
}

char* getmsg(char* buf) {
    char* msg = malloc(MAX_MSG_LENGTH);
    char* msg_beg = strchr(buf + 1, ':');
    if (!msg_beg) {
        log(ERROR, "Failed to find character in msg");
        return NULL;
    }
    
    snprintf(msg, sizeof(msg), "%s", &msg_beg[1]);
    log(DEBUG, "MSG: %s", msg);
    return msg;
}

// TODO
void prepare_buf(char* buf, size_t bytes) {
    // return array of lines
    char* lines = malloc(bytes);

    // create a safety buf to not destroy the original msg buffer
    char* safety_buf = malloc(bytes);
    safety_buf = memcpy(safety_buf, buf, bytes);
    // log(DEBUG, "Safety buf: %.*s", (int)bytes, safety_buf);
    
    int end_reached = false;
    int i = 0;
    while(!i) {
        int line_len = strcspn(buf, "\n");
    }

    free(lines);
    free(safety_buf);
    return;
}

void handle_data(char* buf, size_t bytes, struct config src_conf, struct config dst_conf, SSL* ssl) {
    char* s = NULL;
    prepare_buf(buf, bytes);
    log(DEBUG, "Received %d bytes: %.*s", (int)bytes, (int)bytes, buf);

    buf[bytes] = '\0';
    
    // HANDLE PING
    if (bytes >= 5 && strncmp(buf, "PING ", 5) == 0) {
        log(DEBUG, "Ping :DD");
        buf[bytes] = '\0';
        char response[64];
        snprintf(response, sizeof(response), "PONG %s\r\n", &buf[5]);
        if (tls_send(ssl, response) == ERROR) {
            log(ERROR, "Failed to send ping response");
            return;
        }
        log(DEBUG, "Sent bytes: %s", response);

    }
    // HANDLE MESSAGES
    else if ((s = strstr(buf, "PRIVMSG ")) != NULL) {
        char* user = getusr(buf);
        char* msg = getmsg(buf);
        
        char response[200];
        snprintf(response, sizeof(response), "PRIVMSG %s :[%s (%s)]: %s\r\n", dst_conf.channel, user, src_conf.channel, msg);
        if (tls_send(ssl, response) == ERROR) {
            log(ERROR, "Failed to send PRIVMSG to target");
        }

        if (user != NULL)
            free(user);
        
        if (msg != NULL)
            free(msg);
    }
}

int reconnect(struct srv_info* srv) {
    if (connect(srv->sockfd, (struct sockaddr*)&(srv->srv), sizeof(srv->srv))) {
        goto error;
    }
    
    if (perform_tls_handshake(srv->ssl) == ERROR) {
        log(ERROR, "Failed to perform handshake");
        goto error;
    }

    srv->connection_established = true;
    return 0;
    
 error:
    log(ERROR, "Failed to reconnect with %s", srv->conf.hostname);
    return ERROR;
}

void close_connection(struct srv_info srv) {
    if (!srv.connection_established)
        return;
    
    close_tls_connection(srv.ssl);
    close(srv.sockfd);
    log(WARN, "Closed connection for %s", srv.conf.hostname);
}
