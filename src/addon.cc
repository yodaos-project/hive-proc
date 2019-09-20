#include "addon.hh"
#include <sys/wait.h>
#include <unistd.h>
#include "helper.hh"

#define LOG_TAG "hiveaddon"
#include "logger.h"

namespace hiveproc {

Napi::Value ForkAndSpecialize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::Object process = info[0].As<Napi::Object>();
  uv_loop_t *loop;
  napi_get_uv_event_loop(napi_env(env), &loop);

  int conn_socket = initUnixSocket(HIVE_SOCKET);
  if (conn_socket < 0) {
    return Napi::Number::New(env, conn_socket);
  }

  pid_t _comm_pid = 0;
  for (;;) {
    std::shared_ptr<Caps> comm_caps;
    int comm_socket = poll(conn_socket, comm_caps, _comm_pid);
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
    assert(ret->write(ResponseStatus::Success) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    assert(ret->serialize(buf, buf_size) == buf_size);
    writex(comm_socket, buf, buf_size);
    comm_fd = comm_socket;
    break;
#undef CONTINUE
  }

  comm_pid = _comm_pid;
  YDLogw("hiveproc init, got host(%d), %d", comm_pid, _comm_pid);
  hive__sigchld_start();

  for (;;) {
    hive__checkchld();

    pid_t data_pid;
    std::shared_ptr<Caps> caps;
    int data_socket = poll(conn_socket, caps, data_pid);
    if (data_socket < 0) {
      YDLogv("unexpected err %d(%s)", errno, strerror(errno));
      continue;
    }
    if (data_pid != comm_pid) {
      close(data_socket);
      YDLogv("unmatched command pid(%d) and data pid(%d)", comm_pid, data_pid);
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
    assert(comm_type == CommandType::Fork);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
      assert(close(comm_fd) == 0);
      assert(close(data_socket) == 0);
      assert(close(conn_socket) == 0);

      int stdio = open("/dev/null", O_RDWR);
      assert(dup2(stdio, STDIN_FILENO) != -1);
      assert(dup2(stdio, STDOUT_FILENO) != -1);
      assert(dup2(stdio, STDERR_FILENO) != -1);
      assert(close(stdio) == 0);

      uv_loop_fork(loop);
      SpecializeProcess(process, cwd, argv);
      return Napi::Number::New(env, 0);
    }
    std::shared_ptr<Caps> ret = Caps::new_instance();
    assert(ret->write(ResponseStatus::Success) == CAPS_SUCCESS);
    assert(ret->write(pid) == CAPS_SUCCESS);
    size_t buf_size = ret->binary_size();
    uint8_t buf[buf_size];
    assert(ret->serialize(buf, buf_size) == buf_size);
    writex(data_socket, buf, buf_size);
    YDLogv("forked child(%d), closing connection", pid);
    assert(close(data_socket) == 0);
  }
}

void SpecializeProcess(Napi::Object process, std::string &cwd,
                       std::shared_ptr<Caps> &argv) {
  Napi::Env env = process.Env();
  assert(chdir(cwd.c_str()) == 0);

  auto cwdDescriptor = Napi::PropertyDescriptor::Value(
      "cwd", Napi::String::New(env, cwd),
      napi_property_attributes(napi_enumerable | napi_configurable));
  auto pidDescriptor = Napi::PropertyDescriptor::Value(
      "pid", Napi::Number::New(env, getpid()),
      napi_property_attributes(napi_enumerable | napi_configurable));
  auto ppidDescriptor = Napi::PropertyDescriptor::Value(
      "ppid", Napi::Number::New(env, getppid()),
      napi_property_attributes(napi_enumerable | napi_configurable));
  process.DefineProperties({cwdDescriptor, pidDescriptor, ppidDescriptor});

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
  /* When this function is called, the signal lock must be held. */
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  if (sigfillset(&sa.sa_mask)) {
    abort();
  }
  sa.sa_handler = hive__sigchld;

  /* XXX save old action so we can restore it later on? */
  if (sigaction(SIGCHLD, &sa, NULL)) {
    abort();
  }
}

void hive__sigchld(int) { pending_chld_entry = true; }

void hive__checkchld() {
  pending_chld_entry = false;
  int pending_chld = false;

  pid_t pid;
  int status;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
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
} // namespace hiveproc

NAPI_MODULE_INIT() {
  Napi::Object expobj = Napi::Value(env, exports).As<Napi::Object>();
  expobj.Set("forkAndSpecialize",
             Napi::Function::New(env, hiveproc::ForkAndSpecialize));
  return expobj;
}
