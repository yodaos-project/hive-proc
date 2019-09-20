#include "hiveproc.hh"
#include "helper.hh"
#include <sys/wait.h>
#include <unistd.h>

#include "logger.h"

namespace hive {
Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::Object process = info[0].As<Napi::Object>();

  hive__sigchld_start();

  if (info[1].IsString()) {
    // Fork System Service
    Napi::String entry = info[1].As<Napi::String>();
    pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) {
      std::string cwd;
      std::shared_ptr<Caps> argv;
      SpecializeProcess(process, cwd, argv);
      process.Get("argv").As<Napi::Array>().Set(1, entry);
      return Napi::Number::New(env, 0);
    }
    YDLogi("forked system service: %d", pid);
  }

  int conn_socket = initUnixSocket(HIVE_SOCKET);
  if (conn_socket < 0) {
    return Napi::Number::New(env, conn_socket);
  }

  pid_t _comm_pid = 0;
  for (;;) {
    hive__checkchld();

    std::shared_ptr<Caps> comm_caps;
    int comm_socket = poll(conn_socket, comm_caps, _comm_pid);
    if (comm_socket < 0) {
      continue;
    }
    int32_t comm_type = -1;
#define CONTINUE()                                                             \
  {                                                                            \
    close(comm_socket);                                                        \
    _comm_pid = 0;                                                             \
    continue;                                                                  \
  }
    if (comm_caps->read(comm_type) != CAPS_SUCCESS) {
      YDLogw("hiveproc pending init, failed to parse a init command");
      CONTINUE();
    }
    if (comm_type != CommandType::Init) {
      YDLogw("hiveproc pending init, expecting a init command, but got %d",
             comm_type);
      CONTINUE();
    }

    std::shared_ptr<Caps> ret = Caps::new_instance();
    CHECK(ret->write(ResponseStatus::Success) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    CHECK(ret->serialize(buf, buf_size) == buf_size);
    writex(comm_socket, buf, buf_size);
    comm_fd = comm_socket;
    break;
#undef CONTINUE
  }

  YDLogw("hiveproc init, got host(%d)", _comm_pid);

  for (;;) {
    hive__checkchld();

    pid_t data_pid;
    std::shared_ptr<Caps> caps;
    int data_socket = poll(conn_socket, caps, data_pid);
    if (data_socket < 0) {
      continue;
    }
    if (data_pid != _comm_pid) {
      close(data_socket);
      YDLogv("unmatched command pid(%d) and data pid(%d)", _comm_pid, data_pid);
      continue;
    }

    int32_t comm_type = -1;
    std::string cwd;
    std::shared_ptr<Caps> argv;
    std::shared_ptr<Caps> environs;
#define SAFE_ASSERT(it)                                                        \
  {                                                                            \
    if (!(it)) {                                                               \
      YDLogw(#it ": failed");                                                  \
      close(data_socket);                                                      \
      continue;                                                                \
    }                                                                          \
  }
    SAFE_ASSERT(caps->read(comm_type) == CAPS_SUCCESS);
    SAFE_ASSERT(caps->read(cwd) == CAPS_SUCCESS);
    SAFE_ASSERT(caps->read(argv) == CAPS_SUCCESS);
    SAFE_ASSERT(caps->read(environs) == CAPS_SUCCESS);

#undef SAFE_ASSERT
    // Suicide on mismatched command
    CHECK(comm_type == CommandType::Fork);

    pid_t pid = fork();
    CHECK(pid >= 0);

    if (pid == 0) {
      CHECK(close(comm_fd) == 0);
      CHECK(close(data_socket) == 0);
      CHECK(close(conn_socket) == 0);

      SpecializeProcess(process, cwd, argv);
      return Napi::Number::New(env, 0);
    }
    std::shared_ptr<Caps> ret = Caps::new_instance();
    CHECK(ret->write(ResponseStatus::Success) == CAPS_SUCCESS);
    CHECK(ret->write(pid) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    CHECK(ret->serialize(buf, buf_size) == buf_size);
    writex(data_socket, buf, buf_size);
    YDLogv("forked child(%d), closing connection", pid);
    CHECK(close(data_socket) == 0);
  }
}

void SpecializeProcess(Napi::Object process, std::string &cwd,
                       std::shared_ptr<Caps> &argv) {
  Napi::Env env = process.Env();
  hive__sigchld_stop();

  int stdio = open("/dev/null", O_RDWR);
  CHECK(dup2(stdio, STDIN_FILENO) != -1);
  CHECK(dup2(stdio, STDOUT_FILENO) != -1);
  CHECK(dup2(stdio, STDERR_FILENO) != -1);
  CHECK(close(stdio) == 0);

  uv_loop_t *loop;
  CHECK(napi_get_uv_event_loop(napi_env(env), &loop) == napi_ok);
  CHECK(uv_loop_fork(loop) == 0);

  if (!cwd.empty()) {
    CHECK(chdir(cwd.c_str()) == 0);
    auto cwdDescriptor = Napi::PropertyDescriptor::Value(
        "cwd", Napi::String::New(env, cwd),
        napi_property_attributes(napi_enumerable | napi_configurable));
    process.DefineProperties({cwdDescriptor});
  }

  auto pidDescriptor = Napi::PropertyDescriptor::Value(
      "pid", Napi::Number::New(env, getpid()),
      napi_property_attributes(napi_enumerable | napi_configurable));
  auto ppidDescriptor = Napi::PropertyDescriptor::Value(
      "ppid", Napi::Number::New(env, getppid()),
      napi_property_attributes(napi_enumerable | napi_configurable));
  process.DefineProperties({pidDescriptor, ppidDescriptor});

  if (argv == nullptr) {
    return;
  }
  Napi::Array process_argv = process.Get("argv").As<Napi::Array>();
  Napi::Array child_argv = Napi::Array::New(env, argv->size() + 1);
  Napi::String argv0 = process_argv.Get(uint32_t(0)).As<Napi::String>();
  child_argv.Set(uint32_t(0), argv0);
  for (uint32_t idx = 1; idx <= argv->size(); idx++) {
    int32_t next_type = argv->next_type();
    switch (next_type) {
    case CAPS_MEMBER_TYPE_STRING: {
      std::string val;
      argv->read(val);
      Napi::String jval = Napi::String::New(env, val);
      child_argv.Set(idx, jval);
      break;
    }
    default:
      YDLogw("unsupported type %d", next_type);
    }
  }
  process["argv"] = child_argv;
}

void hive__sigchld_start(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  if (sigfillset(&sa.sa_mask)) {
    abort();
  }
  sa.sa_handler = hive__sigchld;

  if (sigaction(SIGCHLD, &sa, NULL)) {
    abort();
  }
}

void hive__sigchld_stop(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_DFL;

  /* sigaction can only fail with EINVAL or EFAULT; an attempt to deregister a
   * signal implies that it was successfully registered earlier, so EINVAL
   * should never happen.
   */
  if (sigaction(SIGCHLD, &sa, NULL))
    abort();
}

void hive__sigchld(int) { pending_chld_entry = true; }

void hive__checkchld() {
  pending_chld_entry = false;
  int pending_chld = false;

  pid_t pid;
  int status;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if (system_service_pid > 0 && pid == system_service_pid) {
      exit(3);
    }

    pending_chld = true;
    int exit_status = 0;
    int term_signal = 0;
    if (WIFEXITED(status))
      exit_status = WEXITSTATUS(status);

    if (WIFSIGNALED(status))
      term_signal = WTERMSIG(status);
    YDLogv("child process(%d) exited with code(%d) and signal(%d)", pid,
           exit_status, term_signal);
  }

  if (pending_chld && comm_fd > 0) {
    uint8_t data = 0;
    write(comm_fd, &data, 1);
  }
}
} // namespace hive

NAPI_MODULE_INIT() {
  Napi::Object expobj = Napi::Value(env, exports).As<Napi::Object>();
  expobj.Set("forkAndSpecialize",
             Napi::Function::New(env, hive::ForkAndSpecialize));
  return expobj;
}
