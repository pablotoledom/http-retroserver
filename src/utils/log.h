// log.h

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

extern int log_level;

#define LOG_ERROR(fmt, ...) if (log_level >= LOG_LEVEL_ERROR) { printf("ERROR: " fmt "\n", ##__VA_ARGS__); }
#define LOG_WARN(fmt, ...)  if (log_level >= LOG_LEVEL_WARN)  { printf("WARN: "  fmt "\n", ##__VA_ARGS__); }
#define LOG_INFO(fmt, ...)  if (log_level >= LOG_LEVEL_INFO)  { printf("INFO: "  fmt "\n", ##__VA_ARGS__); }
#define LOG_DEBUG(fmt, ...) if (log_level >= LOG_LEVEL_DEBUG) { printf("DEBUG: " fmt "\n", ##__VA_ARGS__); }

#endif // LOG_H
