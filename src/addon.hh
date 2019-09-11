#pragma once

#ifndef HIVE_SOCKET
#define HIVE_SOCKET "/var/run/socket"
#endif

#include "hiveproc.hh"
#define NODE_ADDON_API_DISABLE_DEPRECATED
#define NAPI_EXPERIMENTAL
#define NAPI_VERSION 4
#include "napi.h"
#include "uv.h"

namespace hiveproc
{
Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info);
void SpecializeProcess(Napi::Object process, std::shared_ptr<Caps>& argv);
}
