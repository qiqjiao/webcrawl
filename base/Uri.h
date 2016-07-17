#pragma once

#include <ostream>
#include <string>

namespace base {

struct Uri {
  std::string str;

  std::string scheme;
  std::string username;
  std::string password;
  std::string host;
  int         port = 0;
  std::string path;
  std::string query;
  std::string fragment;

  Uri(const std::string& uri = std::string());
  Uri(const Uri& other) = default;

  Uri &operator=(const std::string& uri) { return *this = Uri(uri); }
  Uri &operator=(const Uri& other) = default;
};

std::ostream &operator<<(std::ostream& os, const Uri& uri);

} // namespace base
