#include "config.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>

static int load_server(dictionary* ini, const char* section, struct config* conf) {
    char key[64];

    snprintf(key, sizeof(key), "%s:hostname", section);
    snprintf(conf->hostname, sizeof(conf->hostname), "%s", iniparser_getstring(ini, key, ""));

    snprintf(key, sizeof(key), "%s:channel", section);
    snprintf(conf->channel, sizeof(conf->channel), "%s", iniparser_getstring(ini, key, ""));

    snprintf(key, sizeof(key), "%s:port", section);
    conf->port = iniparser_getint(ini, key, -1);

    log(DEBUG, "Load [%s]: host=%s channel=%s port=%d", section, conf->hostname, conf->channel, conf->port);

    if (conf->hostname[0] == '\0' || conf->channel[0] == '\0' || conf->port <= 0 || conf->port > 65535) {
        log(ERROR, "[%s] needs hostname, channel and a valid port", section);
        return ERROR;
    }
    return 0;
}

int load_config(const char* path, struct config* conf1, struct config* conf2) {
    dictionary* ini = iniparser_load(path);
    if (ini == NULL) {
        log(ERROR, "Cannot parse file: %s", path);
        return ERROR;
    }

    int ret = 0;
    if (load_server(ini, "Server 1", conf1) == ERROR || load_server(ini, "Server 2", conf2) == ERROR)
        ret = ERROR;

    iniparser_freedict(ini);
    return ret;
}
