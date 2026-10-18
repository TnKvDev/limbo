#include "config.h"
#include "logger.h"

#include <string.h>

// TODO UNDEF values check
int load_config(const char* path, struct config* conf1, struct config* conf2) {
    dictionary* ini = iniparser_load(path);
    if (ini == NULL) {
        log(ERROR, "Cannot parse file: %s", path);
        return ERROR;
    }

    // Dump whole file
    // iniparser_dump(ini, stderr);

    // LOAD VALUES
    strncpy(conf1->hostname, iniparser_getstring(ini, "Server 1:hostname", NULL), sizeof(conf1->hostname));
    log(DEBUG, "Load value s1 hostname: %s", conf1->hostname);

    strncpy(conf2->hostname, iniparser_getstring(ini, "Server 2:hostname", NULL), sizeof(conf2->hostname));
    log(DEBUG, "Load value s2 hostname: %s", conf2->hostname);

    strncpy(conf1->channel, iniparser_getstring(ini, "Server 1:channel", NULL), sizeof(conf1->channel));
    log(DEBUG, "Load value s1 channel: %s", conf1->channel);

    strncpy(conf2->channel, iniparser_getstring(ini, "Server 2:channel", NULL), sizeof(conf2->channel));
    log(DEBUG, "Load value s2_channel: %s", conf2->channel);
    
    conf1->port = iniparser_getint(ini, "Server 1:port", -1);
    log(DEBUG, "Load value s1 port: %d", conf1->port);

    conf2->port = iniparser_getint(ini, "Server 2:port", -1);
    log(DEBUG, "Load value s2 port: %d", conf2->port);

    iniparser_freedict(ini);
    return 0;
}
