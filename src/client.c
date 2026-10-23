#include "client.h"
#include "irc.h"
#include "tls.h"
#include "logger.h"

int connect_to(const char* hostname, int port, struct srv_info* srv, SSL_CTX* ctx) {
    srv->connection_established = false;
    srv->buflen = 0;
    log(DEBUG, "Building connection to %s", hostname);

    // Create socket
    srv->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->sockfd == -1) {
        log(ERROR, "Failed to create a socket for %s", hostname);
        return ERROR;
    }
    log(DEBUG, "Created socket file descriptor for %s", hostname);

    // Fetch information from the host
    srv->srvent = gethostbyname(hostname);
    if(srv->srvent == NULL) {
        log(ERROR, "Failed to fetch host information from %s", hostname);
        goto error;
    }
    log(DEBUG, "Fetched information about %s", hostname);

    // Only clear the sockaddr
    memset(&srv->srv, 0, sizeof(srv->srv));
    memcpy(&(srv->srv.sin_addr.s_addr), srv->srvent->h_addr_list[0], srv->srvent->h_length); // IP
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
        goto error_ssl;
    }
    log(DEBUG, "Set SNI to %s", hostname);

    if (!SSL_set1_host(srv->ssl, hostname)) {
        log(ERROR, "Failed to set certificate verification hostname");
        goto error_ssl;
    }
    log(DEBUG, "Set host to %s", hostname);

    SSL_set_fd(srv->ssl, srv->sockfd);
    log(DEBUG, "Set sockfd");

    // CONNECTION
    if (connect(srv->sockfd, (struct sockaddr*)&(srv->srv), sizeof(srv->srv))) {
        log(ERROR, "Failed to connect to %s", hostname);
        goto error_ssl;
    }

    if (perform_tls_handshake(srv->ssl) == ERROR) {
        log(ERROR, "Failed to perform handshake");
        goto error_ssl;
    }

    srv->connection_established = true;
    return 0;

 error_ssl:
    SSL_free(srv->ssl);
    srv->ssl = NULL;
 error:
    close(srv->sockfd);
    return ERROR;
}

int irc_fill(struct srv_info* srv) {
    size_t got = 0;
    int rc = tls_read(srv->ssl, srv->buf + srv->buflen, (int)(BUF_SIZE - srv->buflen), &got);
    if (rc == TLS_OK)
        srv->buflen += got;
    return rc;
}

int irc_pop_line(struct srv_info* srv, char* line, size_t line_size) {
    char* nl = memchr(srv->buf, '\n', srv->buflen);
    if (nl == NULL) {
        // A single line that fills the whole buffer can never complete: drop it
        if (srv->buflen == BUF_SIZE) {
            log(WARN, "Dropping overlong line from %s", srv->conf.hostname);
            srv->buflen = 0;
        }
        return 0;
    }

    size_t consumed = (size_t)(nl - srv->buf) + 1;
    size_t len = consumed - 1;
    if (len > 0 && srv->buf[len - 1] == '\r')
        len--;
    if (len >= line_size)
        len = line_size - 1;

    memcpy(line, srv->buf, len);
    line[len] = '\0';

    memmove(srv->buf, srv->buf + consumed, srv->buflen - consumed);
    srv->buflen -= consumed;
    return 1;
}

void handle_data(struct srv_info* src, struct srv_info* dst) {
    char line[BUF_SIZE];

    while (irc_pop_line(src, line, sizeof(line))) {
        log(DEBUG, "[%s] %s", src->conf.hostname, line);

        struct irc_msg msg;
        if (parse_irc_line(line, &msg) == ERROR)
            continue;

        switch (classify_msg(&msg)) {
        case PING:
            irc_handle_ping_cmd(src->ssl, &msg);
            break;
        case PRIVMSG:
            irc_handle_privmsg_cmd(src, dst, &msg);
            break;
        case NOTHING_INTERESTING:
        default:
            break;
        }
    }
}

int service_connection(struct srv_info* src, struct srv_info* dst) {
    do {
        int rc = irc_fill(src);
        if (rc == TLS_AGAIN)
            return 0;
        if (rc != TLS_OK) {
            log(WARN, "Lost connection to %s", src->conf.hostname);
            return ERROR;
        }
        handle_data(src, dst);
    } while (SSL_pending(src->ssl) > 0);

    return 0;
}

int reconnect(struct srv_info* srv, SSL_CTX* ctx) {
    unsigned int delay = 5;

    close_connection(*srv);
    srv->connection_established = false;
    srv->buflen = 0;

    for (;;) {
        log(WARN, "Reconnecting to %s in %u seconds", srv->conf.hostname, delay);
        sleep(delay);

        if (connect_to(srv->conf.hostname, srv->conf.port, srv, ctx) == 0) {
            if (register_irc(srv) == 0) {
                log(SUCCESS, "Reconnected to %s", srv->conf.hostname);
                return 0;
            }
            close_connection(*srv);
            srv->connection_established = false;
        }

        if (delay < 60)
            delay *= 2;
    }
}

void close_connection(struct srv_info srv) {
    if (!srv.connection_established)
        return;

    close_tls_connection(srv.ssl);
    close(srv.sockfd);
    log(WARN, "Closed connection for %s", srv.conf.hostname);
}
