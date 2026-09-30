#pragma once

#include <stdio.h>
#include <time.h>

#define SYSM_LOG "[LOG] "
#define SYSM_WARN "\033[0;33m[WARN]\033[0m "
#define SYSM_ERR "\033[0;31m[ERR]\033[0m "

#define SYSM_BUG "[BUG] "

extern int sysm_logger_fd;

#define sysm_log(fmt, ...) dprintf(sysm_logger_fd, "[%lu] [%s:%d] " fmt, time(NULL), __FILE__, __LINE__, ##__VA_ARGS__)

#define sysm_log_ret(retval, fmt, ...) \
        do { \
                sysm_log(fmt, ##__VA_ARGS__); \
                return retval; \
        } while (0)

#define sysm_log_goto(point, fmt, ...)          \
        do {                                    \
                sysm_log(fmt, ##__VA_ARGS__);   \
                goto point;                     \
        } while (0)

static inline void sysm_logger_change_fd(int fd)
{
        sysm_logger_fd = fd;
}