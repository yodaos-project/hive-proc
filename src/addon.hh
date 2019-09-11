#pragma once

#ifndef HIVE_SOCKET
#define HIVE_SOCKET "/var/run/hive.sock"
#endif

#include "hiveproc.hh"
#define NAPI_VERSION 3
#include "napi.h"
#include "uv.h"

namespace hiveproc
{
Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info);
void SpecializeProcess(Napi::Object process, std::shared_ptr<Caps>& argv);
}
