#pragma once

#include <caps.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

namespace hiveproc {
inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte);
inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte);
inline int readCaps(int fildes, std::shared_ptr<Caps> &caps);
} // namespace hiveproc

#include "helper-inl.hh"
