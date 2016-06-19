#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>

#include <curl/multi.h>
#include <json/json.h>

#include "base/Ares.h"
#include "base/EventServer.h"
#include "CrawlContext.h"

namespace crawl {

class Fetcher {
public:
  const static int kMaxTasksCnt = 32768;
  const static int kMaxBodySize = 4096*1024;

  Fetcher();
  ~Fetcher();

  void add(std::shared_ptr<CrawlContext> ctx);

  // {
  //    "tasks" : [ {
  //      "method": "GET",
  //      "uri": "http://www.google.com/",
  //      "headers": ["Accept-Encoding: gzip, deflate", ...]
  //    }, ...]
  // }
  //
  // {
  //    "tasks" : [ {
  //      "id": 1314314545,
  //      "summary": "GET http://www.google.com/"
  //    }, ...]
  // }
  // or
  // { "error": "Invalid uri ..." }
  Json::Value create_tasks_req(const Json::Value &req_body);

  // {
  //    "tasks" : [ {
  //      "id": 1314314545,
  //
  //      "method": "GET",
  //      "uri": "http://www.google.com/",
  //      "headers": ["Accept-Encoding: gzip, deflate", ...]
  //
  //      #"error": "Cannot resolve host",
  //
  //      "redirect": "https://www.google.com/",
  //
  //      "response_code": 200,
  //      "response_reason": "OK",
  //      "response_headers": ["Content-Length: 500"],
  //      "response_body": ".....",
  //    }, ...]
  // }
  Json::Value get_done_tasks_req();

private:
  int socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp);
  int timer_cb(CURLM *multi, long timeout_ms);
  void check_multi_info();

  static int multi_socket_cb(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);
  static int multi_timer_cb(CURLM *multi, long timeout_ms, void *userp);

  base::Ares ares_;
  base::EventServer es_;
  std::thread thread_;

  CURLM *multi_ = nullptr;
  const base::EventServer::Handle *multi_timer_handle_ = nullptr;
  const base::EventServer::Handle *stats_timer_handle_ = nullptr;

  std::vector<std::shared_ptr<CrawlContext>> tasks_;
};

} // namespace crawl
