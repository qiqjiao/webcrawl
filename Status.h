#pragma once

#include <sstream>

namespace base {

class Status {
public:
  enum Code {
    kOk            = 0,
    kError         = 1,
  };

  Status(Code code = kOk, const std::string& msg = "")
      : code_(code), msg_(msg)
  {}

  Code code() const              { return code_; }
  const std::string& msg() const { return msg_; }

  Status &ok()    { code_ = kOk;    msg_.clear(); return *this; }
  Status &error() { code_ = kError; msg_.clear(); return *this; }


  operator bool() const { return code_ == kOk; };

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

inline std::ostream& operator<<(std::ostream& os, const Status& s) {
  os << "[" << s.code() << ":" << (s.msg().empty() ? "OK":s.msg()) << "]";
  return os;
}

} // namespace base
