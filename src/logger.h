#pragma once

#define YDLog_VA(out, msg, ...) \
  fprintf(out, "%s(%s:%d): " msg "\n", LOG_TAG, __FILE__, __LINE__, ##__VA_ARGS__)

// YDLogve(message)
#define YDLogv(...) YDLog_VA(stdout, __VA_ARGS__)

// YDLogw(message)
#define YDLogw(...) YDLog_VA(stderr, __VA_ARGS__)
