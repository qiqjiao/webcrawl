#include "CrawlContext.h"

namespace crawl {

namespace {

Json::Value to_json_str_array(const std::vector<std::string>& array) {
  Json::Value j(Json::arrayValue);
  for (const std::string& v: array) {
    j.append(v);
  }
  return j;
}

std::vector<std::string> from_json_str_array(const Json::Value& obj) {
  std::vector<std::string> vec;
  if (obj.type() != Json::arrayValue) {
    throw std::invalid_argument("Not an array, " + std::to_string((int)obj.type()));
  }
  for (const Json::Value &v: obj) { vec.push_back(v.asString()); }
  return vec;
}

} // namespace

#define SET_INT64(obj, field)  if (field != -1) { obj[#field] = Json::Int64(field); }
#define SET_URI(obj, field)  if (!field.str.empty()) { obj[#field] = field.str; }
#define SET_STRING(obj, field)  if (!field.empty()) { obj[#field] = field; }
#define SET_STRING_ARRAY(obj, field)  if (!field.empty()) { obj[#field] = to_json_str_array(field); }

Json::Value CrawlContext::ToJson() const {
  Json::Value obj(Json::objectValue);
  SET_INT64(obj, id);
  SET_STRING(obj, method);
  SET_URI(obj, uri);
  SET_URI(obj, redirect_uri);
  SET_STRING_ARRAY(obj, req_headers);
  SET_INT64(obj, resp_code);
  SET_STRING_ARRAY(obj, resp_headers);
  SET_STRING(obj, resp_body);
  SET_STRING(obj, error_message);
  return obj;
}

#define GET_INT64(obj, field) if (obj.isMember(#field)) { field = obj[#field].asInt64(); }
#define GET_URI(obj, field) if (obj.isMember(#field)) { field.Init(obj[#field].asString()); }
#define GET_STRING(obj, field) \
    if (obj.isMember(#field) && obj.type() != Json::nullValue) { field = obj[#field].asString(); }
#define GET_STRING_ARRAY(obj, field) \
    if (obj.isMember(#field) && obj[#field].type() != Json::nullValue) { field = from_json_str_array(obj[#field]); }

CrawlContext& CrawlContext::FromJson(const Json::Value& obj) {
  GET_INT64(obj, id);
  GET_STRING(obj, method);
  GET_URI(obj, uri);
  GET_URI(obj, redirect_uri);
  GET_STRING_ARRAY(obj, req_headers);
  GET_INT64(obj, resp_code);
  GET_STRING_ARRAY(obj, resp_headers);
  GET_STRING(obj, resp_body);
  GET_STRING(obj, error_message);
  return *this;
}

} // namespace crawl
