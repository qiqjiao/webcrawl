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
  void task_cb();
  int socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp);
  int timer_cb(CURLM *multi, long timeout_ms);
  void check_multi_info();

  static int multi_socket_cb(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);
  static int multi_timer_cb(CURLM *multi, long timeout_ms, void *userp);

  EventServer es_;
  std::thread thread_;

  bool stopped_ = false;
  std::mutex mutex_;
  const EventServer::Handle *task_handle_ = nullptr;
  std::deque<std::shared_ptr<CrawlContext>> new_tasks_;

  std::vector<std::shared_ptr<CrawlContext>> tasks_;

  CURLM *multi_;
  const EventServer::Handle *multi_timer_handle_ = nullptr;
};
} // namespace crawl
