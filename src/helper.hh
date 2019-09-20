#pragma once

#include <caps.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HIVE_RECV_TIMEOUT
#define HIVE_RECV_TIMEOUT 100
#endif

#define CHECK(ret)                                                             \
  if (!(ret)) {                                                                \
    YDLogw(#ret ": Assertion failed");                                         \
    abort();                                                                   \
  };

namespace hive {
inline int initUnixSocket(const char *pathname);
inline int poll(int conn_socket, std::shared_ptr<Caps> &caps, pid_t &pid);
inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte);
inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte);
inline int readCaps(int fildes, std::shared_ptr<Caps> &caps);
} // namespace hive

#include "helper-inl.hh"
