#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "config.h"
#include "tls.h"
#include "client.h"
#include "irc.h"
#include "logger.h"

struct srv_info s1;
struct srv_info s2;

typedef struct {
    struct srv_info *s1;
    struct srv_info *s2;
} thread_args_t;

/* void test() { */
/*     struct cmd_break_up* cmd_brkup = NULL; */
/*     char buf[] = ":irc.tnkv.dev 003 limbo :This server was created Thu, 08 Oct 2026 20:06:56 UTC"; */

/*     cmd_brkup = break_cmd_up(buf, 80); */
/*     if (cmd_brkup == NULL) { */
/*         log(ERROR, "Failed to break up a command"); */
/*     } */
/*     free(cmd_brkup); */
/*     exit(0); */
/* } */

void* server_loop(void* arg) {
    thread_args_t *args = (thread_args_t *)arg;
    struct srv_info* srv = args->s1;
    struct srv_info* srv2 = args->s2;
    
    size_t readbytes = 0;
    while(1) {
        int ret = tls_read(srv->ssl, srv->buf, BUF_SIZE, &readbytes);
        switch (ret) {
        case TLS_OK: // data received
            handle_data(srv->buf, readbytes, srv->conf, srv2->conf, srv->ssl);
            break;
        case TLS_AGAIN: // wait
            break;
        case TLS_CLOSED: // Peer closed connection
            log(WARN, "Reconnecting..");
            reconnect(srv);
        case ERROR:
            // do nothing
            break;
        }
    }
}

int main(void) {
    /* test(); */
    load_config("conf.ini", &(s1.conf), &(s2.conf));

    SSL_CTX* ctx = create_ctx();
    
    if (connect_to(s1.conf.hostname, s1.conf.port, &s1, ctx) == ERROR) {
        log(ERROR, "Failed to connect to %s", &(s1.conf.hostname));
        goto end;
    }

    if (connect_to(s2.conf.hostname, s2.conf.port, &s2, ctx) == ERROR) {
        log(ERROR, "Failed to connect to %s", &(s2.conf.hostname));
        goto end;
    }

    // TODO FIX REGISTRATION
    // SETUP IRC (nick, username, channels etc etc)
    if (register_irc(s1, s1.conf) == ERROR) {
        log(ERROR, "An error occurred at setup");
        goto end;
    }

    if (register_irc(s2, s2.conf) == ERROR) {
        log(ERROR, "An error occurred at setup");
        goto end;
    }

    pthread_t c1;
    pthread_t c2;
    
    thread_args_t args1 = { &s1, &s2 };
    thread_args_t args2 = { &s2, &s1 };

    // Create thread 1
    if (pthread_create(&c1, NULL, (void *(*)(void *))server_loop, &args1) != 0) {
        log(ERROR, "Failed to create thread for server 1");
        goto end;
    }

    // Create thread 2
    if (pthread_create(&c2, NULL, (void *(*)(void *))server_loop, &args2) != 0) {
        log(ERROR, "Failed to create thread for server 2");
        pthread_cancel(c1);
        goto end;
    }

    pthread_join(c1, NULL);
    pthread_join(c2, NULL);
    
 end:
    close_connection(s1);
    close_connection(s2);
    SSL_CTX_free(ctx);
    return 0;
}
