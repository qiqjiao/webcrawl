#include "Ares.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

namespace base { namespace test {

class AresTest : public testing::Test {};

TEST_F(AresTest, smoke) {
  Ares ares;
  ares.resolve("www.google.com", [](const Ares::AddrList& addrs, const Status &s) {
    for (const auto& addr: addrs) {
      LOG(INFO) << "www.google.com -> " << addr;
    }
  });
  ares.resolve("mail.google.com", [](const Ares::AddrList& addrs, const Status &s) {
    for (const auto& addr: addrs) {
      LOG(INFO) << "mail.google.com -> " << addr;
    }
  });
  sleep(3);
}

}} // namespace base::test
