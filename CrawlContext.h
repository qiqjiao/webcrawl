#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <json/json.h>

#include "base/Uri.h"

namespace crawl {

struct CrawlContext {

  struct SubContext {
    virtual ~SubContext() {}
  };

  long                        id = -1;
  std::string                 summary;

  std::string                 method;
  base::Uri                   uri;
  std::vector<std::string>    req_headers;

  long                        resp_code = -1;
  std::string                 resp_reason;
  std::vector<std::string>    resp_headers;
  std::string                 resp_body;

  bool                        truncated = false;
  std::string                 error_message;
  base::Uri                   redirect_uri;

  // Statistics
  time_t                      start_req_time_ms;
  time_t                      end_resolve_time_ms;
  time_t                      end_req_time_ms;

  // Internal used fields
  std::string                 ip;
  bool                        done = false;

  std::unordered_map<std::string, std::unique_ptr<SubContext>> sub_ctx;

  Json::Value to_json() const;
  void from_json(const Json::Value& json);
};

} // namespace crawl
