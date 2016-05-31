#pragma once

#include <memory>
#include <thread>
#include <vector>

#include <curl/multi.h>

#include "Common.h"
#include "EventServer.h"

namespace crawl {
class Fetcher {
public:
  Fetcher();
  ~Fetcher();

  bool add(std::shared_ptr<CrawlContext> ctx);

  int socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp);
  int timer_cb(CURLM *multi, long timeout_ms);
  void cache_resolve(std::shared_ptr<CrawlContext> ctx);

  void run() {
    es_.loop();
  }

private:
  void check_multi_info();

  std::vector<std::shared_ptr<CrawlContext>> tasks_;
  std::thread thread_;
  EventServer es_;
  const EventServer::Handle *timer_handle_ = nullptr;
  CURLM *multi_;
  curl_slist *resolve_lsit_ = NULL;
};
} // namespace crawl
