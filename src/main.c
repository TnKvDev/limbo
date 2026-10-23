#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "tls.h"
#include "client.h"
#include "irc.h"
#include "logger.h"

struct srv_info s1;
struct srv_info s2;

int main(void) {
    if (load_config("conf.ini", &(s1.conf), &(s2.conf)) == ERROR) {
        return 1;
    }

    SSL_CTX* ctx = create_ctx();
    if (ctx == NULL) {
        return 1;
    }

    if (connect_to(s1.conf.hostname, s1.conf.port, &s1, ctx) == ERROR) {
        log(ERROR, "Failed to connect to %s", s1.conf.hostname);
        goto end;
    }

    if (connect_to(s2.conf.hostname, s2.conf.port, &s2, ctx) == ERROR) {
        log(ERROR, "Failed to connect to %s", s2.conf.hostname);
        goto end;
    }

    // SETUP IRC (nick, username, channels etc etc)
    if (register_irc(&s1) == ERROR) {
        log(ERROR, "An error occurred at setup (%s)", s1.conf.hostname);
        goto end;
    }

    if (register_irc(&s2) == ERROR) {
        log(ERROR, "An error occurred at setup (%s)", s2.conf.hostname);
        goto end;
    }
    
    int ret = 1;
    struct srv_info* srvs[2] = { &s1, &s2 };

    for (;;) {
        struct pollfd fds[2] = {
            {
                .fd = s1.sockfd,
                .events = POLLIN
            },

            {
                .fd = s2.sockfd,
                .events = POLLIN
            },
        };

        if (poll(fds, 2, -1) < 0) {
            if (errno == EINTR)
                continue;
            log(ERROR, "poll() failed");
            break;
        }

        for (int i = 0; i < 2; i++) {
            if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
                continue;

            if (service_connection(srvs[i], srvs[1 - i]) == ERROR)
                reconnect(srvs[i], ctx);
        }
    }

 end:
    close_connection(s1);
    close_connection(s2);
    SSL_CTX_free(ctx);
    return ret;
}
