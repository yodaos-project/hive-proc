#pragma once

#include <stdint.h>
#include <unistd.h>

namespace hiveproc {
inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte);
inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte);
} // namespace hiveproc

#include "helper-inl.hh"
