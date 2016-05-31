#include "Common.h"

namespace crawl {

std::ostream& operator<<(std::ostringstream& os, const Status& s) {
  os << "[" << s.code() << ":" << s.msg() << "]";
  return os;
}

} // namespace crawl
