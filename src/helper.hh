#pragma once

#include <caps.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define assert(ret)                                                            \
  if (!(ret)) {                                                                \
    YDLogw(#ret ": assertion failed");                                         \
    abort();                                                                   \
  };

namespace hiveproc {
inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte);
inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte);
inline int readCaps(int fildes, std::shared_ptr<Caps> &caps);
} // namespace hiveproc

#include "helper-inl.hh"
