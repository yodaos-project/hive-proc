#pragma once

#include "helper.hh"
#define NAPI_VERSION 3
#include "napi.h"
#include "uv.h"

namespace hive {
Napi::Value Request(const Napi::CallbackInfo &info);
Napi::Array caps2js(Napi::Env env, std::shared_ptr<Caps> &caps);
} // namespace hive
