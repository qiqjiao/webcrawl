#include "Uri.h"

#include <regex>

namespace base {

void Uri::Init(const std::string& u) {
  static const std::regex uri_regex(
      // scheme  ://  usr :pwd  @   host    :port        /path         ?query     #fragment
      "^([a-z]+)://(([^:]+)(:([^@]+))?@)?([^/:]+)(:([0-9]+))?(/[^\\?#]*)?(\\?([^#]*))?(#(.*))?$");

  str = u;
  std::smatch smatch;
  if (!std::regex_match(str, smatch, uri_regex)) {
    throw std::invalid_argument("Invalid uri [" + str + "]");
  }

  scheme   = smatch[1].str();
  username = smatch[3].str();
  password = smatch[5].str();
  host     = smatch[6].str();
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
}

std::ostream &operator<<(std::ostream& os, const Uri& uri) {
  os << uri.str;
  return os;
}

} // namespace base
