#include "Ares.h"

#include <glog/logging.h>

Ares::Ares() {
  uv_loop_init(&uv_loop_);

  uv_async_handle_.data = this;
  uv_async_init(&uv_loop_, &uv_async_handle_, &Ares::uv_async_cb);

  thread_ = std::thread([this]() {
    uv_run(&uv_loop_, UV_RUN_DEFAULT);
  });
}

Ares::~Ares() {}

void Ares::resolve(const std::string& hostname, ResolveCb cb) {
  std::lock_guard<std::mutex> l(mutex_);
  auto &info = resolve_infos_[hostname];
  info.ares = this;
  info.hostname = hostname;
  info.cbs.push_back(std::move(cb));
  info.uv_handle.data = &info;
  uv_async_send(&uv_async_handle_);
}

void Ares::uv_async_cb(uv_async_t* handle) {
  // https://nikhilm.github.io/uvbook/networking.html
  Ares *ares = static_cast<Ares*>(handle->data);
  std::lock_guard<std::mutex> l(ares->mutex_);
  for (auto& entry: ares->resolve_infos_) {
    if (entry.second.in_processing) { continue; }
    entry.second.in_processing = true;

    struct addrinfo hints;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags    = 0;

    int r = uv_getaddrinfo(&ares->uv_loop_, &entry.second.uv_handle,
                           Ares::uv_on_resolved_cb,
                           entry.first.data(), NULL, &hints);
    if (r != 0) {
      LOG(ERROR) << "getaddrinfo call error " << uv_err_name(r);
    }
  }
}

void Ares::uv_on_resolved_cb(uv_getaddrinfo_t* req, int status,
                             struct addrinfo* res)
{
  AddrList addrs;
  const char *error = nullptr;
  ResolveInfo *info = static_cast<ResolveInfo*>(req->data);

  if (status < 0) {
    error = uv_err_name(status);
  } else {
    for (struct addrinfo* rp = res; rp != NULL; rp = rp->ai_next) {
      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) rp->ai_addr, addr, 16);
      addrs.push_back(addr);
      VLOG(1) << info->hostname << ":" << addr;
    }
  }

  std::lock_guard<std::mutex> l(info->ares->mutex_);
  for (const auto& cb: info->cbs) { cb(addrs, error); }
  info->cbs.clear();

  uv_freeaddrinfo(res);
}

