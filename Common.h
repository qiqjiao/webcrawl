#pragma once

#include <sstream>

#include "Utils.h"

namespace crawl {

class Status {
public:
  enum Code {
    kOK            = 0,
    kError         = 1,
  };

  Status(Code code = kOK, const std::string& msg = "")
      : code_(code), msg_(msg)
  {}

  Code code() const { return code_; }
  const std::string& msg() const { return msg_; }

  operator bool() const { return code_ != kOK; };

  template <class T>
  Status& operator<<(const T& t) {
    std::ostringstream oss;
    oss << t;
    msg_ += oss.str();
    return *this;
  }

private:
  Code code_;
  std::string msg_;
};

std::ostream& operator<<(std::ostringstream& os, const Status& s);

struct CrawlContext {
  Url         url;
  int         ip;
  std::string ip_str;
  long        response_code;
  std::string response_headers;
  std::string response_page;
};

} // namespace crawl
