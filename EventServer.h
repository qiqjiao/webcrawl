#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

struct event;
struct event_base;

class EventServer {
public:
  enum Flags {
    READ    = 0x2,
    WRITE   = 0x4,
    PERSIST = 0x10,
  };
  struct Handle;

  using AsyncCb = std::function<void(void)>;
  using TimerCb = std::function<void(void)>;
  using FdCb  = std::function<void(int fd, int events)>;

  EventServer();
  ~EventServer();

  void loop();
  void exit(int ms = 0);

  void add_async(const AsyncCb& cb);
  const Handle *add_timer(const TimerCb& cb, int flags = 0);
  const Handle *add_fd(int fd, int flags, const FdCb& cb);
  void remove(const Handle* h);

  void start(const Handle* h, int64_t ms = -1);
  void stop(const Handle* h);

private:
  static void ev_cb(int fd, short flags, void *arg);

  struct event_base *eb_;
  std::unordered_map<const Handle*, std::shared_ptr<Handle>> handles_;

  std::mutex mutex_;
  const Handle *async_handle_;
  std::deque<AsyncCb> async_callbacks_;
};
