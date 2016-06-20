#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Status.h"

namespace base {

class Resolver {
public:
  using AddrList = std::vector<std::string>;
  using ResolveCb = std::function<void(const AddrList &addrs, const Status &s)>;

  Resolver();
  ~Resolver();

  void resolve(const std::string& name, const ResolveCb& cb);

private:
  struct Impl;
  static void ev_cb(int fd, short flags, void *arg);

  std::unique_ptr<Impl> impl_;
};

} // namespace base
