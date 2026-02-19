
#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define LOG_BASE(level, fmt, ...) \
    do { \
        if (level <= LOG_LEVEL) { \
            fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_ERROR(fmt, ...) \
    LOG_BASE(LOG_LEVEL_ERROR, "[ERROR] %s:%d: " fmt, \
             __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    LOG_BASE(LOG_LEVEL_WARN, "[WARN ] %s:%d: " fmt, \
             __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    LOG_BASE(LOG_LEVEL_INFO, "[INFO ] %s:%d: " fmt, \
             __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
    LOG_BASE(LOG_LEVEL_DEBUG, "[DEBUG] %s:%d: " fmt, \
             __FILE__, __LINE__, ##__VA_ARGS__)

#endif
