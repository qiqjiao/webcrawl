#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Status.h"

namespace base {

class EventServer;

class Ares {
public:
  using AddrList = std::vector<std::string>;
  using ResolveCb = std::function<void(const AddrList &addrs, const Status &s)>;

  Ares();
  ~Ares();

  void resolve(const std::string& name, const ResolveCb& cb);

private:
  struct Impl;

  void do_work();

  std::unique_ptr<Impl> impl_;
};

} // namespace base
