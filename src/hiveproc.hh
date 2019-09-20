#pragma once

#ifndef HIVE_SOCKET
#define HIVE_SOCKET "/var/run/hive.sock"
#endif

#include "helper.hh"
#define NAPI_DISABLE_CPP_EXCEPTIONS
#define NAPI_VERSION 3
#include "napi.h"
#include "uv.h"

namespace hive {
enum CommandType { Init = 0, Fork };
enum ResponseStatus {
  Success = 0,
};

pid_t system_service_pid = 0;
int comm_fd = -1;
bool pending_chld_entry = false;

void hive__sigchld(int sig);
void hive__sigchld_start(void);
void hive__sigchld_stop(void);
void hive__checkchld();
Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info);
void SpecializeProcess(Napi::Object process, std::string &cwd,
                       std::shared_ptr<Caps> &argv);
} // namespace hive
