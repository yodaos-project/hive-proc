#include <stdint.h>
#include <sys/errno.h>
#include <unistd.h>

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
} // namespace hiveproc
