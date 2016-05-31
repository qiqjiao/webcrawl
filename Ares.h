#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <uv.h>

class Ares {
public:
  using AddrList = std::vector<std::string>;
  using ResolveCb = std::function<void(const AddrList& addrs, const char *error)>;

  Ares();
  ~Ares();

  void resolve(const std::string& hostname, ResolveCb cb);

private:
  struct ResolveInfo {
    Ares *ares;
    bool in_processing = false;
    std::string hostname;
    uv_getaddrinfo_t uv_handle;
    std::deque<ResolveCb> cbs;
  };

  static void uv_async_cb(uv_async_t* handle);
  static void uv_on_resolved_cb(uv_getaddrinfo_t* req, int status,
                                struct addrinfo* res);

  std::thread thread_;
  std::mutex  mutex_;
  std::unordered_map<std::string, ResolveInfo> resolve_infos_;
  uv_loop_t   uv_loop_;
  uv_async_t  uv_async_handle_;
};
