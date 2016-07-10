#pragma once

#include <ostream>
#include <string>

namespace base {

struct Uri {
  void Init(const std::string& uri);

  std::string str;

  std::string scheme;
  std::string username;
  std::string password;
  std::string host;
  int         port = 0;
  std::string path;
  std::string query;
  std::string fragment;

  Uri& operator=(const std::string& u) { Init(u); }
};

std::ostream &operator<<(std::ostream& os, const Uri& uri);

} // namespace base
