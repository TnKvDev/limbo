#ifndef CONFIG_H
#define CONFIG_H

#include "iniparser.h"

struct config {
    char hostname[64];
    char channel[32];
    int  port;
};

int load_config(const char* path, struct config* conf1, struct config* conf2);

#endif // CONFIG_H
