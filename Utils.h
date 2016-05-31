#pragma once

#include <ostream>
#include <string>

namespace crawl {

struct Url {
  bool Init(const std::string& u);

  std::string url;

  std::string scheme;
  std::string username;
  std::string password;
  std::string hostname;
  int         port = 0;
  std::string path;
  std::string query;
  std::string fragment;
};

std::ostream &operator<<(std::ostream& os, const Url& url);

} // namespace crawl
