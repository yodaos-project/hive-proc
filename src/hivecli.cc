#include "hivecli.hh"
#include "logger.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace hive {
Napi::Value Request(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  std::string hivesock = info[0].As<Napi::String>().Utf8Value();
  napi_value buf = info[1];
  bool keep_socket = false;
  if (info[2].IsBoolean()) {
    keep_socket = info[2].As<Napi::Boolean>().Value();
  }
  uint8_t *buf_data;
  size_t buf_length;
  CHECK(napi_get_buffer_info(env, buf, (void **)&buf_data, &buf_length) ==
        napi_ok);

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    Napi::Error::New(env, "socket error").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, hivesock.c_str());
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    Napi::Error::New(env, "connect error").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  writex(fd, buf_data, buf_length);

  std::shared_ptr<Caps> caps;
  if (readCaps(fd, caps) < 0) {
    close(fd);
    Napi::Error::New(env, "read caps error").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Array res = caps2js(env, caps);
  if (keep_socket) {
    res["fd"] = Napi::Number::New(env, fd);
  } else {
    close(fd);
  }
  return res;
}

Napi::Array caps2js(Napi::Env env, std::shared_ptr<Caps> &caps) {
  Napi::Array ret = Napi::Array::New(env, caps->size());
  for (int idx = 0; idx < caps->size(); idx++) {
    int32_t next_type = caps->next_type();
    switch (next_type) {
#define READ_NUMBER(type, decl)                                                \
  case CAPS_MEMBER_TYPE_##type: {                                              \
    decl val;                                                                  \
    caps->read(val);                                                           \
    Napi::Number jval = Napi::Number::New(env, val);                           \
    ret[idx] = jval;                                                           \
    break;                                                                     \
  }
      READ_NUMBER(INTEGER, int32_t)
      READ_NUMBER(FLOAT, float)
      READ_NUMBER(LONG, int64_t)
      READ_NUMBER(DOUBLE, double)
    case CAPS_MEMBER_TYPE_STRING: {
      std::string val;
      caps->read(val);
      Napi::String jval = Napi::String::New(env, val);
      ret[idx] = jval;
      break;
    }
    default:
      YDLogw("unsupported type %d", next_type);
    }
  }
  return ret;
}
} // namespace hive

NAPI_MODULE_INIT() {
  Napi::Object expobj = Napi::Value(env, exports).As<Napi::Object>();
  expobj.Set("request", Napi::Function::New(env, hive::Request));
  return expobj;
}
