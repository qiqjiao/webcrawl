#include "Ares.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

class AresTest : public testing::Test {};

TEST_F(AresTest, smoke) {
  Ares ares;
  ares.resolve("www.google.com", [](const Ares::AddrList& addrs, const char *error) {
    for (const auto& addr: addrs) {
      LOG(INFO) << addr;
    }
  });
  ares.resolve("mail.google.com", [](const Ares::AddrList& addrs, const char *error) {
    for (const auto& addr: addrs) {
      LOG(INFO) << addr;
    }
  });

  sleep(10);
}
