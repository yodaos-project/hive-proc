#pragma once
#include <time.h>

#define YDLog_VA(out, level, msg, ...)                                         \
  {                                                                            \
    time_t timer;                                                              \
    char buffer[26];                                                           \
    struct tm *tm_info;                                                        \
    time(&timer);                                                              \
    tm_info = localtime(&timer);                                               \
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);                        \
    fprintf(out, "%s/%s (%s:%d): " msg "\n", level, buffer, __FILE__,          \
            __LINE__, ##__VA_ARGS__);                                          \
  }

// YDLogvv(message)
#define YDLogv(...) YDLog_VA(stdout, "V", __VA_ARGS__)
// YDLogvi(message)
#define YDLogi(...) YDLog_VA(stdout, "I", __VA_ARGS__)

// YDLogw(message)
#define YDLogw(...) YDLog_VA(stderr, "W", __VA_ARGS__)
// YDLoge(message)
#define YDLoge(...) YDLog_VA(stderr, "E", __VA_ARGS__)
