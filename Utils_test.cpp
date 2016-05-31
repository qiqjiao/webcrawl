#include "Utils.h"

#include <gtest/gtest.h>

namespace crawl { namespace test {

class UtilsTest : public testing::Test {

};

TEST_F(UtilsTest, Url) {
  Url u;

  auto verify = [](const std::vector<std::string> &v, const Url &u) {
    EXPECT_EQ(v[0], u.scheme);
    EXPECT_EQ(v[1], u.username);
    EXPECT_EQ(v[2], u.password);
    EXPECT_EQ(v[3], u.hostname);
    EXPECT_EQ(v[4], std::to_string(u.port));
    EXPECT_EQ(v[5], u.path);
    EXPECT_EQ(v[6], u.query);
    EXPECT_EQ(v[7], u.fragment);
  };

  u.Init("foo://username:password@example.com:8042/over/there/index.dtb?type=animal&name=narwhal#nose");
  verify({"foo", "username", "password", "example.com", "8042",
          "/over/there/index.dtb", "type=animal&name=narwhal", "nose"}, u);
  // submatch 0: foo://username:password@example.com:8042/over/there/index.dtb?type=animal&name=narwhal#nose
  // submatch 1: foo
  // submatch 2: username:password@
  // submatch 3: username
  // submatch 4: :password
  // submatch 5: password
  // submatch 6: example.com
  // submatch 7: :8042
  // submatch 8: 8042
  // submatch 9: /over/there/index.dtb
  // submatch 10: ?type=animal&name=narwhal
  // submatch 11: type=animal&name=narwhal
  // submatch 12: #nose
  // submatch 13: nose
  u.Init("http://example.com/over/there/index.dtb");
  verify({"http", "", "", "example.com", "80", "/over/there/index.dtb", "", ""}, u);
}

}} // namespace crawl::test