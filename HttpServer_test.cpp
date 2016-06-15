#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <glog/logging.h>

namespace base { namespace test {

class HttpServerTest : public testing::Test {
};

TEST_F(HttpServerTest, async) {
  HttpServer svr("0.0.0.0", 8888);

  svr.AddHandle("/", [](const HttpServer::Request &req, HttpServer::Response *resp) {
    VLOG(1) << req.method << " " << req.uri;
    for (const auto& e: req.headers) { VLOG(1) << e.first << ":" << e.second; }
    VLOG(2) << req.body;

    resp->body = req.body;
    resp->headers = req.headers;
  });
  svr.AddHandle("/stop", [&svr](const HttpServer::Request &req, HttpServer::Response *resp) {
    VLOG(1) << "Stopping server";
    svr.Stop();
  });

  EXPECT_TRUE(svr.Run());
}

}} // namespace base::test
