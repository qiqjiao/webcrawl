#include "CrawlContext.h"

namespace crawl {

#define SET_URI(json, uri) if (!uri.str.empty()) json[#uri] = uri.str
#define SET_BOOL(json, field) json[#field] = field
#define SET_LONG(json, field) if (field != -1) json[#field] = Json::Int64(field)
#define SET_STRING(json, field) if (!field.empty()) json[#field] = field
#define SET_STRING_LIST(json, field) do { \
	for (const std::string& v: field) json[#field].append(v); } while (0)

Json::Value CrawlContext::to_json() const {
  Json::Value json;

  SET_LONG(json, id);
  SET_STRING(json, summary);

  SET_STRING(json, method);
  SET_URI(json, uri);
  SET_STRING_LIST(json, req_headers);

  SET_LONG(json, resp_code);
  SET_STRING(json, resp_reason);
  SET_STRING_LIST(json, resp_headers);
  SET_STRING(json, resp_body);

  SET_BOOL(json, truncated);
  SET_STRING(json, error_message);
  SET_URI(json, redirect_uri);

  return json;
}

#define GET_URI(json, field) if (json.isMember(#field)) field = json[#field].asString()
#define GET_LONG(json, field) if (json.isMember(#field)) field = json[#field].asInt64()
#define GET_STRING(json, field) if (json.isMember(#field)) field = json[#field].asString()
#define GET_STRING_LIST(json, field) do { \
	if (json.isMember(#field)) { \
		for (const Json::Value& v: json[#field]) { \
			field.push_back(v.asString()); \
		} \
	} \
	} while (0)

void CrawlContext::from_json(const Json::Value& json) {
  GET_STRING(json, method);
  GET_URI(json, uri);
  GET_STRING_LIST(json, req_headers);
}

} // namespace crawl
