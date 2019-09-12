#include "caps.h"
#include <stdint.h>
#include <sys/errno.h>
#include <unistd.h>
#define LOG_TAG "helper-inl"
#include "logger.h"

namespace hiveproc {

inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte) {
  ssize_t r = 0;
  while (r < nbyte) {
    ssize_t res = read(fildes, buf + r, nbyte - r);
    if (res == -1) {
      if (errno == EINTR) {
        continue;
      }
      return res;
    }
    r += res;
  }
  return r;
}

inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte) {
  ssize_t r = 0;
  while (r < nbyte) {
    ssize_t res = write(fildes, buf + r, nbyte - r);
    if (res == -1) {
      if (errno == EINTR) {
        continue;
      }
      return res;
    }
    r += res;
  }
  return r;
}

inline int readCaps(int fildes, std::shared_ptr<Caps> &caps) {
#define CHECKEQ(actual, expected)                                              \
  {                                                                            \
    auto ret = actual;                                                         \
    if (ret != expected) {                                                     \
      YDLogw(#actual ": failed with actual(%d), expected: %d", ret, expected); \
      return -1;                                                               \
    };                                                                         \
  }

  uint64_t header;
  size_t header_length = sizeof(uint64_t);
  CHECKEQ(readx(fildes, (uint8_t *)&header, header_length), header_length);
  uint32_t version;
  uint32_t length;
  CHECKEQ(Caps::binary_info(&header, &version, &length), CAPS_SUCCESS);
  YDLogv("Header from peer: length=%u, version=%u", length, version);

  uint8_t data[length];
  memcpy(data, &header, header_length);
  CHECKEQ(readx(fildes, data + header_length, length - header_length),
          length - header_length);
  CHECKEQ(Caps::parse(data, length, caps, true), CAPS_SUCCESS);

  return 0;
}
} // namespace hiveproc
