#include "helper.hh"
#include <unistd.h>
#include <iostream>

#include "logger.h"

using namespace hive;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    YDLogw("Usage: canary-in-the-hive <sock-path>");
    return 1;
  }
  YDLogv("listening on %s", argv[1]);
  int conn_socket = initUnixSocket(argv[1]);
  if (conn_socket < 0)
  {
    YDLogw("failed to listen on %s: %d", argv[1], conn_socket);
    return 1;
  }

  for (;;)
  {
    pid_t pid;
    std::shared_ptr<Caps> caps;
    int data_socket = poll(conn_socket, caps, pid);
    CHECK(data_socket >= 0);

    std::string target;
    CHECK(caps->read(target) == CAPS_SUCCESS);

    std::shared_ptr<Caps> ret = Caps::new_instance();
    CHECK(ret->write(target) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    CHECK(ret->serialize(buf, buf_size, static_cast<uint32_t>(0x80)) == buf_size);
    writex(data_socket, buf, buf_size);
    close(data_socket);
  }
}
