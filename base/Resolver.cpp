#include "Resolver.h"

#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <arpa/inet.h>
#include <netdb.h>
#include <glog/logging.h>

#include <event2/dns.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "Utils.h"

namespace base {

struct ResolveInfo {
  enum State { kNew, kExpired, kInProcessing, kDone };

  Resolver *resolver = nullptr;

  std::string name;
  std::deque<Resolver::ResolveCb> cbs;

  time_t expire = 0;
  Status status;
  Resolver::AddrList addrs;

  State state = kNew;
};

struct Resolver::Impl {
  volatile bool stopped = false;

  struct event_base *evbase = nullptr;
  struct evdns_base *evdns = nullptr;
  struct event *notify_ev = nullptr;

  std::mutex mutex;
  std::thread thread;
  std::deque<ResolveInfo*> tasks;
  std::unordered_map<std::string/*name*/, ResolveInfo> resolve_infos;
};

Resolver::Resolver(): impl_(new Impl) {
  CHECK_NE(evthread_use_pthreads(), -1);

  impl_->evbase = event_base_new();
  impl_->evdns  = evdns_base_new(impl_->evbase, 1/*evdns_base_new*/);

  timeval tv = {1, 0};
  impl_->notify_ev = event_new(impl_->evbase, -1, EV_PERSIST, &Resolver::ev_cb, this);
  CHECK_EQ(event_add(impl_->notify_ev, &tv), 0);

  impl_->thread = std::thread([this]() {
    CHECK_NE(event_base_dispatch(this->impl_->evbase), -1); 
  });
}

Resolver::~Resolver() {
  event_free(impl_->notify_ev);
  CHECK_NE(event_base_loopexit(impl_->evbase, nullptr), -1);
  impl_->thread.join();
  evdns_base_free(impl_->evdns, 0/*fail_requests*/);
  event_base_free(impl_->evbase);
}

void Resolver::resolve(const std::string& name, const ResolveCb& cb) {
  VLOG(1) << "Resolving name [" << name << "]";

  std::lock_guard<std::mutex> l(impl_->mutex);
  auto &info = impl_->resolve_infos[name];

  if (info.state == ResolveInfo::kDone && info.expire < base::now_in_secs()) {
    VLOG(1) << "Record [" << name << "] expires " << base::now_in_secs() - info.expire;
    info.state = ResolveInfo::kExpired;
  }

  switch (info.state) {
    case ResolveInfo::kNew:
      info.resolver = this;
      info.name = name;
    case ResolveInfo::kExpired:
      info.state = ResolveInfo::kInProcessing;
      info.cbs.push_back(cb);
      impl_->tasks.push_back(&info);
      event_active(impl_->notify_ev, 0, 0);
      break;
    case ResolveInfo::kInProcessing:
      info.cbs.push_back(cb);
      break;
    case ResolveInfo::kDone:
      cb(info.addrs, info.status);
      break;
  }
}

void Resolver::ev_cb(int fd, short flags, void *arg) {
  auto resolver = static_cast<Resolver*>(arg);

  evdns_callback_type res_cb = [](int result, char type, int count, int ttl,
		                  void *addresses, void *arg) {
    ResolveInfo *info = static_cast<ResolveInfo*>(arg);

    std::lock_guard<std::mutex> l(info->resolver->impl_->mutex);
    VLOG(1) << "Done with resolving name [" << info->name << "," << ttl << "]";
    CHECK_EQ(type, DNS_IPv4_A);
    switch (result) {
      case DNS_ERR_NONE:
        info->status.ok();
	info->expire = base::now_in_secs() + ttl;
        info->addrs.clear();
	for (int i = 0; i < count; ++i) {
          char ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, (char*)addresses + i * 4, ip, sizeof(ip));
          info->addrs.push_back(ip);
	}
        break;
      default:
        info->status.error() << evdns_err_to_string(result);
        break;
    }
    for (auto const &cb: info->cbs) {
      cb(info->addrs, info->status);
    }
    info->cbs.clear();
    info->state = ResolveInfo::kDone;
  };

  std::lock_guard<std::mutex> l(resolver->impl_->mutex);
  for (auto task: resolver->impl_->tasks) {
    auto r = evdns_base_resolve_ipv4(resolver->impl_->evdns,
      	                       task->name.c_str(), 0, res_cb, task);
    CHECK(r != nullptr);
  }
  resolver->impl_->tasks.clear();
}

} // namespace base
