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
  using SubCtxMap = std::unordered_map<std::string, std::unique_ptr<SubContext>>;

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
  std::string                 summary;

  // Internal used fields
  SubCtxMap                   sub_ctx;
  std::string                 ip;
  bool                        done = false;

  Json::Value ToJson() const;
  CrawlContext& FromJson(const Json::Value& obj);
};

} // namespace crawl
