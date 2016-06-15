#pragma once

#include <ostream>
#include <string>

namespace base {

struct Uri {
  bool Init(const std::string& uri);

  std::string uri;

  std::string scheme;
  std::string username;
  std::string password;
  std::string hostname;
  int         port = 0;
  std::string path;
  std::string query;
  std::string fragment;
};

std::ostream &operator<<(std::ostream& os, const Uri& uri);

} // namespace base
