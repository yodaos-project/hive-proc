#include "addon.hh"
#include <unistd.h>

#define LOG_TAG "hiveaddon"
#include "logger.h"

namespace hiveproc
{

Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  Napi::Object process = info[0].As<Napi::Object>();
  uv_loop_t *loop;
  napi_get_uv_event_loop(napi_env(env), &loop);

  int conn_socket = initUnixSocket(HIVE_SOCKET);
  if (conn_socket < 0)
  {
    return Napi::Number::New(env, conn_socket);
  }

  for (;;)
  {
    std::shared_ptr<Caps> caps;
    int data_socket = poll(conn_socket, caps);

    std::shared_ptr<Caps> argv;
    std::shared_ptr<Caps> environs;
    // TODO: Safe assert
    assert(caps->read(argv) == CAPS_SUCCESS);
    assert(caps->read(environs) == CAPS_SUCCESS);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0)
    {
      assert(close(data_socket) == 0);
      assert(close(conn_socket) == 0);
      uv_loop_fork(loop);
      SpecializeProcess(process, argv);
      return Napi::Number::New(env, 0);
    }
    std::shared_ptr<Caps> ret = Caps::new_instance();
    assert(ret->write(pid) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    assert(ret->serialize(buf, buf_size) == buf_size);
    writex(data_socket, buf, buf_size);
    YDLogv("forked child(%d), closing connection", pid);
    assert(close(data_socket) == 0);
  }
}

void SpecializeProcess(Napi::Object process, std::shared_ptr<Caps> &argv)
{
  Napi::Env env = process.Env();
  auto pidDescriptor =
      Napi::PropertyDescriptor::Value("pid",
                                      Napi::Number::New(env, getpid()),
                                      napi_property_attributes(napi_enumerable | napi_configurable));
  auto ppidDescriptor =
      Napi::PropertyDescriptor::Value("ppid",
                                      Napi::Number::New(env, getppid()),
                                      napi_property_attributes(napi_enumerable | napi_configurable));
  process.DefineProperties({pidDescriptor, ppidDescriptor});

  Napi::Array process_argv = process.Get("argv").As<Napi::Array>();
  Napi::Array child_argv = Napi::Array::New(env, argv->size() + 1);
  Napi::String argv0 = process_argv.Get(uint32_t(0)).As<Napi::String>();
  child_argv.Set(uint32_t(0), argv0);

  for (uint32_t idx = 1; idx <= argv->size(); idx++)
  {
    int32_t next_type = argv->next_type();
    switch (next_type)
    {
    case CAPS_MEMBER_TYPE_STRING:
    {
      std::string val;
      argv->read(val);
      Napi::String jval = Napi::String::New(env, val);
      child_argv.Set(idx, jval);
    }
    default:
      YDLogw("unsupported type %d", next_type);
    }
  }
  process["argv"] = child_argv;
}
} // namespace hiveproc

NAPI_MODULE_INIT()
{
  Napi::Object expobj = Napi::Value(env, exports).As<Napi::Object>();
  expobj.Set("forkAndSpecialize", Napi::Function::New(env, hiveproc::ForkAndSpecialize));
  return expobj;
}
