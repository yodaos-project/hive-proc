#include "hiveproc.hh"
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define LOG_TAG "hiveproc"
#include "logger.h"

namespace hiveproc {

int initUnixSocket(char *pathname) {
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
#define SOCKET_ASSERT(it)                                                      \
  {                                                                            \
    auto ret = it;                                                             \
    if (!(ret)) {                                                              \
      YDLogw(#it ": failed with ret(%d), err: %d(%s)", ret, errno,             \
             strerror(errno));                                                 \
      close(data_socket);                                                      \
      return -1;                                                               \
    };                                                                         \
  }
#define SOCKET_CHECKEQ(actual, expected)                                       \
  {                                                                            \
    auto ret = actual;                                                         \
    if (ret != expected) {                                                     \
      YDLogw(#actual ": failed with actual(%d), expected: %d", ret, expected); \
      close(data_socket);                                                      \
      return -1;                                                               \
    };                                                                         \
  }

  int data_socket = accept(conn_socket, NULL, NULL);
  assert(data_socket >= 0);
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

  uint64_t header;
  size_t header_length = sizeof(uint64_t);
  SOCKET_CHECKEQ(readx(data_socket, (uint8_t *)&header, header_length),
                 header_length);
  uint32_t version;
  uint32_t length;
  SOCKET_CHECKEQ(Caps::binary_info(&header, &version, &length), CAPS_SUCCESS);
  YDLogv("Header from peer: length=%u, version=%u", length, version);

  uint8_t data[length];
  memcpy(data, &header, header_length);
  SOCKET_CHECKEQ(
      readx(data_socket, data + header_length, length - header_length),
      length - header_length);
  SOCKET_CHECKEQ(Caps::parse(data, length, caps, true), CAPS_SUCCESS);

#undef SOCKET_CHECKEQ
#undef SOCKET_ASSERT
  return data_socket;
}

} // namespace hiveproc
