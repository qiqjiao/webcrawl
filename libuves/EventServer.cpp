#include "EventServer.h"

#include <glog/logging.h>


EventServer::EventServer() : thread_id_(std::this_thread::get_id())
{
  CHECK_EQ(0, uv_loop_init(&uv_loop_));

    async_handle_ = new AsyncHandle;
    async_handle_->es = this;
    async_handle_->uv_handle.data = async_handle_;
    CHECK_EQ(0, uv_async_init(&uv_loop_, &async_handle_->uv_handle,
                              &EventServer::uv_async_cb));

}

EventServer::~EventServer() {
  CHECK_EQ(0, uv_loop_close(&uv_loop_));
}

void EventServer::start() {
  thread_id_ = std::this_thread::get_id();
  CHECK_EQ(0, uv_run(&uv_loop_, UV_RUN_DEFAULT));
}

void EventServer::stop() {
  add_async([this]() {
    CHECK_EQ(std::this_thread::get_id(), thread_id_);
    uv_walk(&uv_loop_, EventServer::uv_walk_cb, nullptr);

    timer_handles_.clear();
    poll_handles_.clear();
    async_handle_ = nullptr;
  });
}

void EventServer::add_async(AsyncCb cb) {
  std::lock_guard<std::mutex> l(mutex_);

  if (async_handle_ == nullptr) {
    async_handle_ = new AsyncHandle;
    async_handle_->es = this;
    async_handle_->uv_handle.data = async_handle_;
    CHECK_EQ(0, uv_async_init(&uv_loop_, &async_handle_->uv_handle,
                              &EventServer::uv_async_cb));
  }

  async_queue_.push_back(std::move(cb));
  CHECK_EQ(0, uv_async_send(&async_handle_->uv_handle));
}

int64_t EventServer::add_timer(TimerCb &&cb) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  TimerHandle *es_handle = new TimerHandle;
  es_handle->id = reinterpret_cast<int64_t>(es_handle);
  es_handle->es = this;
  es_handle->cb = std::move(cb);
  es_handle->uv_handle.data = es_handle;

  timer_handles_[es_handle->id] = es_handle;
  CHECK_EQ(0, uv_timer_init(&uv_loop_, &es_handle->uv_handle));

  return es_handle->id;
};

void EventServer::start_timer(int64_t handle, int64_t timeout, int64_t repeat) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = timer_handles_[handle];

  CHECK_EQ(0, uv_timer_start(&es_handle->uv_handle, &EventServer::uv_timer_cb,
                             timeout, repeat));
}

void EventServer::stop_timer(int64_t handle) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = timer_handles_[handle];

  CHECK_EQ(0, uv_timer_stop(&es_handle->uv_handle));
}

void EventServer::remove_timer(int64_t handle) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = timer_handles_[handle];

  CHECK_EQ(0, uv_timer_stop(&es_handle->uv_handle));
  uv_close(reinterpret_cast<uv_handle_t*>(&es_handle->uv_handle),
           &EventServer::uv_close_cb);

  timer_handles_.erase(handle);
}

void EventServer::add_poll(int desc, PollCb &&cb) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = new PollHandle;
  es_handle->cb = cb;
  es_handle->es = this;
  es_handle->uv_handle.data = es_handle;
  poll_handles_[desc] = es_handle;

  CHECK_EQ(0, uv_poll_init_socket(&uv_loop_, &es_handle->uv_handle, desc));
}

void EventServer::start_poll(int desc, int events) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = poll_handles_[desc];

  CHECK_EQ(0, uv_poll_start(&es_handle->uv_handle, events,
                            &EventServer::uv_poll_cb));
}

void EventServer::stop_poll(int desc) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = poll_handles_[desc];

  CHECK_EQ(0, uv_poll_stop(&es_handle->uv_handle));
  uv_close(reinterpret_cast<uv_handle_t*>(&es_handle->uv_handle),
           &EventServer::uv_close_cb);

  poll_handles_.erase(desc);
}

void EventServer::remove_poll(int desc) {
  CHECK_EQ(std::this_thread::get_id(), thread_id_);

  auto es_handle = poll_handles_[desc];

  uv_close(reinterpret_cast<uv_handle_t*>(&es_handle->uv_handle),
           &EventServer::uv_close_cb);
  poll_handles_.erase(desc);
}

void EventServer::uv_async_cb(uv_async_t* handle) {
  auto es_handle = static_cast<AsyncHandle*>(handle->data);

  CHECK_EQ(std::this_thread::get_id(), es_handle->es->thread_id_);

  std::deque<AsyncCb> cbs;
  {
    std::lock_guard<std::mutex> l(es_handle->es->mutex_);
    es_handle->es->async_queue_.swap(cbs);
  }
  for (auto& cb: cbs) { cb(); }
}

void EventServer::uv_timer_cb(uv_timer_t *handle) {
  auto es_handle = static_cast<TimerHandle*>(handle->data);

  CHECK_EQ(std::this_thread::get_id(), es_handle->es->thread_id_);

  es_handle->cb();
}

void EventServer::uv_poll_cb(uv_poll_t* handle, int status, int events) {
  auto es_handle = static_cast<PollHandle*>(handle->data);

  CHECK_EQ(std::this_thread::get_id(), es_handle->es->thread_id_);

  auto error = status < 0 ? uv_strerror(status) : nullptr;
  es_handle->cb(events, error);
}

void EventServer::uv_close_cb(uv_handle_t* handle) {
  delete static_cast<Handle*>(handle->data);
}

void EventServer::uv_walk_cb(uv_handle_t* handle, void* arg) {
  uv_close(handle, EventServer::uv_close_cb);
}
