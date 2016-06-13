#include "EventServer.h"

#include <glog/logging.h>
#include <event2/event.h>

struct EventServer::Handle : public std::enable_shared_from_this<EventServer::Handle> {
  enum Type { kTimer, kFd };

  EventServer *es;
  event       *ev;
  Type         type;
  TimerCb      timer_cb;
  FdCb         fd_cb;
};

EventServer::EventServer() {
  eb_ = event_base_new();
}

EventServer::~EventServer() {
  auto handles = handles_;
  for (auto &e: handles) { remove(e.first); }
  event_base_free(eb_);
}

void EventServer::loop() {
  CHECK_EQ(event_base_dispatch(eb_), 0);
}

void EventServer::exit(int ms) {
  timeval tv = {ms/1000, ms%1000*1000};
  CHECK_EQ(event_base_loopexit(eb_, &tv), 0);
}

const EventServer::Handle *EventServer::add_timer(const EventServer::TimerCb& cb, int flags) {
  Handle *h   = new Handle;
  h->es       = this;
  h->type     = Handle::kTimer;
  h->timer_cb = cb;
  h->ev       = event_new(eb_, -1, flags, &EventServer::ev_cb, h);
  handles_[h].reset(h);
  return h;
}

const EventServer::Handle *EventServer::add_fd(int fd, int flags, const EventServer::FdCb& cb) {
  Handle *h   = new Handle;
  h->es       = this;
  h->type     = Handle::kFd;
  h->fd_cb    = cb;
  h->ev       = event_new(eb_, fd, flags, &EventServer::ev_cb, h);
  handles_[h].reset(h);
  return h;
}

void EventServer::remove(const Handle* h) {
  auto hh = const_cast<Handle*>(h);
  CHECK_EQ(event_del(hh->ev), 0);
  event_free(hh->ev);
  handles_.erase(hh);
}

void EventServer::start(const Handle* h, int64_t ms) {
  if (ms < 0) {
    CHECK_EQ(event_add(h->ev, nullptr), 0);
  } else {
    timeval tv = {ms/1000, ms%1000*1000};
    CHECK_EQ(event_add(h->ev, &tv), 0);
  }
}

void EventServer::stop(const Handle* h) {
  CHECK_EQ(event_del(h->ev), 0);
}

void EventServer::ev_cb(evutil_socket_t fd, short flags, void *arg) {
  auto h = static_cast<Handle*>(arg)->shared_from_this();
  switch (h->type) {
    case Handle::kTimer:
      h->timer_cb();
      break;

    case Handle::kFd: {
      h->fd_cb(fd, flags);
      break;
    }
  }
}
