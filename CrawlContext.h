#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/Uri.h"

namespace crawl {

struct CrawlContext {

  struct SubContext {
    virtual ~SubContext() {}
  };

  long                        id = -1;

  std::string                 method;
  base::Uri                   uri;
  base::Uri                   redirect_uri;
  std::vector<std::string>    req_headers;

  long                        resp_code = -1;
  std::vector<std::string>    resp_headers;
  std::string                 resp_body;

  // Statistics
  time_t                      start_req_time_ms;
  time_t                      end_resolve_time_ms;
  time_t                      end_req_time_ms;

  bool                        truncated = false;
  std::string                 error_message;

  // Internal used fields
  std::string                 ip;
  bool                        done = false;

  std::unordered_map<std::string, std::unique_ptr<SubContext>> sub_ctx;
};

} // namespace crawl
