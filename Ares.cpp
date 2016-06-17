#include "Ares.h"

#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <arpa/inet.h>
#include <netdb.h>
#include <ares.h>
#include <glog/logging.h>

namespace base {

namespace {

struct ResolveInfo {
  enum State { kNew, kInProcessing, kDone };

  Ares *ares = nullptr;

  std::string name;
  std::deque<Ares::ResolveCb> cbs;

  Status resolve_status;
  Ares::AddrList addrs;

  volatile State state = kNew;
};

} // namespace

struct Ares::Impl {
  bool stopped = false;
  std::mutex mutex;
  std::thread thread;
  ares_channel channel;
  std::deque<ResolveInfo*> new_tasks;
  std::unordered_map<std::string/*name*/, ResolveInfo> resolve_infos;
};

Ares::Ares(): impl_(new Impl) {
  CHECK_EQ(ares_library_init(ARES_LIB_INIT_ALL), 0);

  struct ares_options options;
  options.sock_state_cb = [](void *data, int s, int read, int write) {
    Ares *ares = static_cast<Ares*>(data);
  };
  options.sock_state_cb_data = this;
  int optmask = ARES_OPT_SOCK_STATE_CB;
  CHECK_EQ(ares_init_options(&impl_->channel, &options, optmask), ARES_SUCCESS);

  impl_->thread = std::thread([this]() { this->do_work(); });
}

Ares::~Ares() {
  impl_->stopped = true;
  impl_->thread.join();
  ares_destroy(impl_->channel);
  ares_library_cleanup();
}

void Ares::resolve(const std::string& name, const ResolveCb& cb) {
  VLOG(1) << "Resolving name [" << name << "]";

  std::lock_guard<std::mutex> l(impl_->mutex);
  auto &info = impl_->resolve_infos[name];
  switch (info.state) {
    case ResolveInfo::kNew:
      info.ares = this;
      info.name = name;
      info.cbs.push_back(cb);
      impl_->new_tasks.push_back(&info);
      break;
    case ResolveInfo::kInProcessing:
      info.cbs.push_back(cb);
      break;
    case ResolveInfo::kDone:
      cb(info.addrs, info.resolve_status);
      break;
  }
}

void Ares::do_work() {
  ares_host_callback ares_cb = [](void *arg, int status, int timeouts, struct hostent *hostent) {
    ResolveInfo *info = static_cast<ResolveInfo*>(arg);

    std::lock_guard<std::mutex> l(info->ares->impl_->mutex);

    VLOG(1) << "Done with resolving name [" << info->name << "]";
    info->state = ResolveInfo::kDone;
    switch (status) {
      case ARES_SUCCESS: // The host lookup completed successfully.
        info->resolve_status.ok();
        // http://www.gnu.org/software/libc/manual/html_node/Host-Names.html
        for (int i = 0; hostent->h_addr_list[i]; ++i) {
          char ip[INET6_ADDRSTRLEN];
          inet_ntop(hostent->h_addrtype, hostent->h_addr_list[i], ip, sizeof(ip));
          info->addrs.push_back(ip);
        }
        break;
      case ARES_ENOTIMP: // The ares library does not know how to find addresses of type family .
      case ARES_EBADNAME: // The hostname name is composed entirely of numbers and periods, but is not a valid representation of an Internet address.
      case ARES_ENOTFOUND: // The address addr was not found.
      case ARES_ENOMEM: // Memory was exhausted.
      case ARES_ECANCELLED: // The query was cancelled.
      case ARES_EDESTRUCTION: // The name service channel channel is being destroyed; the query will not be completed. 
        info->resolve_status.error() << "Resolve failed";
        break;
      default:
        info->resolve_status.error() << "Resolve failed with unknown error";
        break;
    }
    for (auto const &cb: info->cbs) {
      cb(info->addrs, info->resolve_status);
    }
    info->cbs.clear();
  };

  while (!impl_->stopped) {
    {
      std::lock_guard<std::mutex> l(impl_->mutex);
      for (auto task: impl_->new_tasks) {
        task->state = ResolveInfo::kInProcessing;
        ares_gethostbyname(impl_->channel, task->name.c_str(), AF_INET, ares_cb, task);
      }
      impl_->new_tasks.clear();
    }

    struct timeval *tvp, tv {0, 10*1000};
    tvp = ares_timeout(impl_->channel, &tv, &tv);

    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds); FD_ZERO(&write_fds);
    int nfds = ares_fds(impl_->channel, &read_fds, &write_fds);
    CHECK_NE(select(nfds, &read_fds, &write_fds, NULL, tvp), -1);
    ares_process(impl_->channel, &read_fds, &write_fds);
  }
}

} // namespace base
