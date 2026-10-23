#ifndef TLS_H
#define TLS_H

#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>

#define TLS_OK       0
#define TLS_AGAIN   -1
#define TLS_CLOSED  -2
#define TLS_ERR     -3

SSL_CTX* create_ctx(void);

int perform_tls_handshake(SSL* ssl);

int tls_send(SSL* ssl, const char* msg);
int tls_read(SSL* ssl, char* buf, int num, size_t* readbytes);

void close_tls_connection(SSL* ssl);

#endif // TLS_H
