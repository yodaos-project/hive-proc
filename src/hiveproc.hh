#pragma once

#include <caps.h>

#ifndef HIVE_RECV_TIMEOUT
#define HIVE_RECV_TIMEOUT 100
#endif

namespace hiveproc {
int initUnixSocket(char *pathname);
int poll(int conn_socket, std::shared_ptr<Caps> &caps, pid_t &pid);
ssize_t readx(int fildes, uint8_t *buf, size_t nbyte);
ssize_t writex(int fildes, uint8_t *buf, size_t nbyte);
} // namespace hiveproc
