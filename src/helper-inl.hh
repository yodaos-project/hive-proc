#include "caps.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "logger.h"

namespace hive {

int initUnixSocket(const char *pathname) {
#ifdef __APPLE__
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
#else
  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#endif
  if (fd < 0)
    return -1;
#ifdef __APPLE__
  auto f = fcntl(fd, F_GETFD);
  f |= FD_CLOEXEC;
  fcntl(fd, F_SETFD, f);
#endif
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, pathname);
  unlink(pathname);
  if (::bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    YDLogw("bind to %s failed: %d(%s)", pathname, errno, strerror(errno));
    ::close(fd);
    return -2;
  }
  YDLogv("bound to %s", pathname);
  listen(fd, 10);
  return fd;
}

int poll(int conn_socket, std::shared_ptr<Caps> &caps, pid_t &pid) {
#define SOCKET_CHECKEQ(actual, expected)                                       \
  {                                                                            \
    if (actual != expected) {                                                  \
      YDLogw("Assertion failed: " #actual "");                                 \
      close(data_socket);                                                      \
      return -1;                                                               \
    };                                                                         \
  }

  int data_socket = accept(conn_socket, NULL, NULL);
  if (data_socket < 0) {
    return data_socket;
  }
#ifdef __APPLE__
  socklen_t len = sizeof(pid_t);
  SOCKET_CHECKEQ(
      getsockopt(data_socket, SOCK_STREAM, LOCAL_PEERPID, &pid, &len), 0);
#else
  socklen_t len = sizeof(struct ucred);
  struct ucred ucred;
  SOCKET_CHECKEQ(getsockopt(data_socket, SOL_SOCKET, SO_PEERCRED, &ucred, &len),
                 0);
  pid = ucred.pid;
#endif
  YDLogv("Credentials from peer: pid=%ld", (long)pid);

  if (readCaps(data_socket, caps) < 0) {
    close(data_socket);
    return -1;
  }

#undef SOCKET_CHECKEQ
  return data_socket;
}

ssize_t readx(int fildes, uint8_t *buf, size_t nbyte) {
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

ssize_t writex(int fildes, uint8_t *buf, size_t nbyte) {
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

int readCaps(int fildes, std::shared_ptr<Caps> &caps) {
#define CHECKEQ(actual, expected)                                              \
  {                                                                            \
    if (actual != expected) {                                                  \
      YDLogw("Assertion failed: " #actual "");                                 \
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
} // namespace hive
