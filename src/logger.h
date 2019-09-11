#pragma once
#include <time.h>

#define YDLog_VA(out, msg, ...)                                                \
  {                                                                            \
    time_t timer;                                                              \
    char buffer[26];                                                           \
    struct tm *tm_info;                                                        \
    time(&timer);                                                              \
    tm_info = localtime(&timer);                                               \
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);                        \
    fprintf(out, "%s %s(%s:%d): " msg "\n", buffer, LOG_TAG, __FILE__,         \
            __LINE__, ##__VA_ARGS__);                                          \
  }

// YDLogve(message)
#define YDLogv(...) YDLog_VA(stdout, __VA_ARGS__)

// YDLogw(message)
#define YDLogw(...) YDLog_VA(stderr, __VA_ARGS__)
