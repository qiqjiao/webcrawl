#include "Uri.h"

#include <regex>
#include <iostream>

namespace base {

bool Uri::Init(const std::string& u) {
  static const std::regex uri_regex(
      // scheme  ://  usr :pwd  @   host    :port        /path         ?query     #fragment
      "^([a-z]+)://(([^:]+)(:([^@]+))?@)?([^/:]+)(:([0-9]+))?(/[^\\?#]*)?(\\?([^#]*))?(#(.*))?$");

  uri = u;
  std::smatch smatch;
  if (!std::regex_match(uri, smatch, uri_regex)) { return false; }

  scheme   = smatch[1].str();
  username = smatch[3].str();
  password = smatch[5].str();
  hostname = smatch[6].str();
  path     = smatch[9].str().empty() ? "/" : smatch[9].str();
  query    = smatch[11].str();
  fragment = smatch[13].str();

  if (! smatch[8].str().empty()) {
    port = std::stoi(smatch[8].str());
  } else if (scheme == "http") {
    port = 80;
  } else if (scheme == "https") {
    port = 443;
  }

  return true;
}

std::ostream &operator<<(std::ostream& os, const Uri& uri) {
  os << uri.uri;
  return os;
}

} // namespace base
