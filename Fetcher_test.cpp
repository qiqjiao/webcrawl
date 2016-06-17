#include "Fetcher.h"

#include <fstream>

#include <glog/logging.h>
#include <gtest/gtest.h>

//#include "Ares.h"

namespace crawl { namespace test {

class FetcherTest : public testing::Test {

};
DEFINE_int32(cnt, 200, "");
DEFINE_int32(runtime, 80, "");
DEFINE_string(url, "", "");

TEST_F(FetcherTest, smoke) {
  //Ares ares;
  Fetcher fetcher;
 
  if (!FLAGS_url.empty()) {
    auto ctx = std::make_shared<CrawlContext>();
    fetcher.add(ctx);
    ctx->uri.Init(FLAGS_url);
    VLOG(2) << ctx->uri.str;
  } else {
    std::string line;
    //std::ifstream ifs("testsites");
    std::ifstream ifs("top500sites");
    getline(ifs, line);
    std::vector<std::shared_ptr<CrawlContext>> ctxs;
    int i = 0, resolved = 0;
    while (getline(ifs, line)) {
      size_t s = line.find('"'), e = line.rfind('"');
      auto ctx = std::make_shared<CrawlContext>();
      //EXPECT_TRUE(ctx->url.Init("http://www." + line.substr(s + 1, e - s - 1)));
      ctx->uri.Init("http://" + line.substr(s + 1, e - s - 1));
      //EXPECT_TRUE(ctx->url.Init("http://www.163.com/"));
      //EXPECT_TRUE(ctx->url.Init("http://www.huzide.com/"));
      //EXPECT_TRUE(ctx->url.Init("https://www.google.com/"));
      VLOG(2) << ctx->uri.str;
      //auto ares_cb =  [&fetcher, ctx, &resolved](const Ares::AddrList& addrs, const char *error) {
      //  if (addrs.empty() || error != nullptr) {
      //    LOG(ERROR) << "Failed to get ip for " << ctx->url.hostname << ", " << error;
      //    return;
      //  }
      //  VLOG(1) << "Adding dns cache " << ctx->url.hostname << ":" << addrs[0];
      //  ctx->ip_str = addrs[0];
      //  //fetcher.cache_resolve(ctx);
      //  fetcher.add(ctx);
      //  ++resolved;
      //};
      //ares.resolve(ctx->url.hostname, ares_cb);
      fetcher.add(ctx);
      //ctxs.push_back(ctx);
      if (++i == FLAGS_cnt) { break; }
    }
  }
  //sleep(5);
  //for (auto c: ctxs) {
  //  fetcher.add(c);
  //}
  //fetcher.run();
  sleep(FLAGS_runtime);
  LOG(INFO) << "Test is done";
}

}} // namespace crawl::test
