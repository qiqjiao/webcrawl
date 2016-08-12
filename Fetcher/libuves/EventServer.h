#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <uv.h>

class EventServer {
public:
  enum PollEventType {
    READABLE = 1,
    WRITABLE = 2,
  };

  using AsyncCb = std::function<void(void)>;
  using TimerCb = std::function<void(void)>;
  using PollCb  = std::function<void(int events, const char *error)>;

  EventServer();
  ~EventServer();

  void start();
  void stop();

  void add_async(AsyncCb cb);

  int64_t add_timer(TimerCb &&cb);
  void remove_timer(int64_t handle);
  void start_timer(int64_t handle, int64_t timeout, int64_t repeat = 0);
  void stop_timer(int64_t handle);

  void add_poll(int desc, PollCb &&cb);
  void start_poll(int desc, int events);
  void stop_poll(int desc);
  void remove_poll(int desc);

private:
  struct Handle {
    EventServer *es;
    virtual ~Handle() {}
  };

  struct AsyncHandle: public Handle {
    uv_async_t uv_handle;
  };

  struct TimerHandle: public Handle {
    int64_t      id;
    TimerCb      cb;
    uv_timer_t   uv_handle;
  };

  struct PollHandle: public Handle {
    PollCb       cb;
    uv_poll_t    uv_handle;
  };

  static void uv_async_cb(uv_async_t* handle);
  static void uv_timer_cb(uv_timer_t* handle);
  static void uv_poll_cb(uv_poll_t* handle, int status, int events);
  static void uv_close_cb(uv_handle_t* handle);
  static void uv_walk_cb(uv_handle_t* handle, void* arg);

  std::thread::id     thread_id_;
  std::mutex          mutex_;
  std::deque<AsyncCb> async_queue_;
  std::unordered_map<int64_t/*id*/, TimerHandle*> timer_handles_;
  std::unordered_map<int/*fd*/, PollHandle*>  poll_handles_;

  uv_loop_t  uv_loop_;
  AsyncHandle *async_handle_ = nullptr;
};
