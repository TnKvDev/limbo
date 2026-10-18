#include "tls.h"
#include "logger.h"

SSL_CTX* create_ctx(void) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL) {
        log(ERROR, "Failed to create new SSL context");
        return NULL;
    }

    // Verify peer's certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // Use default certificate stores
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        log(ERROR, "Failed to set default trusted certificate store");
        return NULL;
    }

    // Minimum version TLSv1.2
    if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
        log(ERROR, "Failed to set minimum TLS protocol version");
        return NULL;
    }

    return ctx;
}

int perform_tls_handshake(SSL* ssl) {
    if (SSL_connect(ssl) < 1) {
        log(ERROR, "Failed to perform the TLS handshake");
        return ERROR;
    }
    log(DEBUG, "TLS handshake completed");
    return 0;
}

int tls_send(SSL* ssl, const char* msg) {
    int len = (int)strlen(msg);
    int n = SSL_write(ssl, msg, len);
    if (n <= 0) {
        int err = SSL_get_error(ssl, n);
        log(ERROR, "SSL_write failed (SSL error %d)", err);
        ERR_print_errors_fp(stderr);
        return ERROR;
    }
    
    return 0;
}

int tls_read(SSL* ssl, char* buf, int num, size_t* readbytes) {
    *readbytes = 0;
    if(SSL_read_ex(ssl, buf, num, readbytes) == 1) {
        return TLS_OK;
    }
    
    int err = SSL_get_error(ssl, 0);
    switch (err) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        return TLS_AGAIN;
    case SSL_ERROR_ZERO_RETURN: // Peer sent close notify
        return TLS_CLOSED;
    default:
        log(ERROR, "SSL_read failed (SSL error %d)", err);
        ERR_print_errors_fp(stderr);
        return ERROR;
    }
    
    return 0;
}

void close_tls_connection(SSL* ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
    log(DEBUG, "Closed TLS Connection");
}
