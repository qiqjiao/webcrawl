#include "Utils.h"

#include <sys/time.h>

namespace base {

int64_t now_in_usecs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

int64_t now_in_ms() {
  return now_in_usecs() / 1000;
}

int64_t now_in_secs() {
  return now_in_usecs() / 1000000;
}

} // namespace base
