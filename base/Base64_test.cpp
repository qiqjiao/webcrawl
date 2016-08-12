#include "Base64.h"

#include <gtest/gtest.h>

namespace base { namespace test {

class Base64Test : public testing::Test {};

TEST_F(Base64Test, smoke) {
  EXPECT_EQ("",         base64_encode(""));
  EXPECT_EQ("YQ==",     base64_encode("a"));
  EXPECT_EQ("YWE=",     base64_encode("aa"));
  EXPECT_EQ("YWFh",     base64_encode("aaa"));
  EXPECT_EQ("YWFhYQ==", base64_encode("aaaa"));
  EXPECT_EQ("YWFhYWE=", base64_encode("aaaaa"));
  EXPECT_EQ("YWFhYWFh", base64_encode("aaaaaa"));

  EXPECT_EQ("",         base64_decode(""));
  EXPECT_EQ("a",        base64_decode("YQ=="));
  EXPECT_EQ("aa",       base64_decode("YWE="));
  EXPECT_EQ("aaa",      base64_decode("YWFh"));
  EXPECT_EQ("aaaa",     base64_decode("YWFhYQ=="));
  EXPECT_EQ("aaaaa",    base64_decode("YWFhYWE="));
  EXPECT_EQ("aaaaaa",   base64_decode("YWFhYWFh"));

  EXPECT_EQ(std::string(1, '\0'),     base64_decode("AA=="));
  EXPECT_EQ(std::string(2, '\0'),     base64_decode("AAA="));
  EXPECT_EQ(std::string(3, '\0'),     base64_decode("AAAA"));
  EXPECT_EQ(std::string(4, '\0'),     base64_decode("AAAAAA=="));
  EXPECT_EQ(std::string(5, '\0'),     base64_decode("AAAAAAA="));
  EXPECT_EQ(std::string(6, '\0'),     base64_decode("AAAAAAAA"));
}

}} // namespace base::test
