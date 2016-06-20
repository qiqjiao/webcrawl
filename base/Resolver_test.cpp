#include "Resolver.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

namespace base { namespace test {

class ResolverTest : public testing::Test {};

TEST_F(ResolverTest, smoke) {
  Resolver resolver;
  int calls = 0;
  for (int i = 0; i < 10; ++i) {
    resolver.resolve("www.google.com", [&calls](const Resolver::AddrList& addrs, const Status &s) {
      ++calls;
      for (const auto& addr: addrs) {
        LOG(INFO) << "www.google.com -> " << addr;
      }
    });
    resolver.resolve("mail.google.com", [&calls](const Resolver::AddrList& addrs, const Status &s) {
      ++calls;
      for (const auto& addr: addrs) {
        LOG(INFO) << "mail.google.com -> " << addr;
      }
    });
    sleep(2);
  }
  EXPECT_EQ(20, calls);
}

}} // namespace base::test
