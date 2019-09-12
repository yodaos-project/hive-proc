#include <stdint.h>
#include <unistd.h>

namespace hiveproc {

inline ssize_t readx(int fildes, uint8_t *buf, size_t nbyte) {
  ssize_t r = 0;
  while (r < nbyte) {
    r += read(fildes, buf + r, nbyte - r);
  }
  return r;
}

inline ssize_t writex(int fildes, uint8_t *buf, size_t nbyte) {
  ssize_t r = 0;
  while (r < nbyte) {
    r += write(fildes, buf + r, nbyte - r);
  }
  return r;
}
} // namespace hiveproc
