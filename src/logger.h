#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define DBG

typedef enum LogLevel {
    DEBUG,
    WARN,
    ERROR,
    SUCCESS,
}LogLevel;

// ANSI color escape codes
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define log_error(str, ...) \
    printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] [%s:%d] >>> " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define log_success(str, ...) \
    printf("[" ANSI_COLOR_GREEN "SUCCESS" ANSI_COLOR_RESET "] [%s:%d] >>> " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define log_warning(str, ...) \
    printf("[" ANSI_COLOR_YELLOW "WARN" ANSI_COLOR_RESET "] [%s:%d] >>> " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#ifdef DBG
#define log_debug(str, ...) \
    printf("[" ANSI_COLOR_BLUE "DEBUG" ANSI_COLOR_RESET "] [%s:%d] >>> " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(str, ...) ((void)0) // Do nothing
#endif

#define log(lvl, str, ...) \
    if (lvl == WARN) \
        log_warning(str, ##__VA_ARGS__); \
    else if (lvl == ERROR) \
        log_error(str, ##__VA_ARGS__); \
    else if (lvl == SUCCESS) \
        log_success(str, ##__VA_ARGS__); \
    else if (lvl == DEBUG) \
        log_debug(str, ##__VA_ARGS__); \

#endif //LOGGER_H
