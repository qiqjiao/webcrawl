#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>

#include <curl/multi.h>

#include "Common.h"
#include "EventServer.h"

namespace crawl {
class Fetcher {
public:
  Fetcher();
  ~Fetcher();

  bool add(std::shared_ptr<CrawlContext> ctx);

private:
  int socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp);
  int timer_cb(CURLM *multi, long timeout_ms);
  void check_multi_info();

  static int multi_socket_cb(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);
  static int multi_timer_cb(CURLM *multi, long timeout_ms, void *userp);

  EventServer es_;
  std::thread thread_;
  std::vector<std::shared_ptr<CrawlContext>> tasks_;
  CURLM *multi_ = nullptr;
  const EventServer::Handle *multi_timer_handle_ = nullptr;
  const EventServer::Handle *stats_timer_handle_ = nullptr;
};
} // namespace crawl
