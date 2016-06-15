#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Uri.h"

struct event_base;
struct evhttp;
struct evhttp_bound_socket;

namespace base {

class HttpServer {
public:
  struct Request {
    std::string  method;
    Uri          uri;
    std::string  body;
    std::unordered_map<std::string, std::string> headers;
  };

  struct Response {
    int          code = 200;
    std::string  reason = "OK";
    std::string  body;
    std::unordered_map<std::string, std::string> headers;
  };

  using HandleCb = std::function<void(const Request &req, Response *resp)>;

  HttpServer(const std::string& address, int port);
  ~HttpServer();

  bool Run();
  void Stop();

  void AddHandle(const std::string& path, const HandleCb& cb);

private:
  struct HandleCtx;

  const std::string address_;
  const int port_;

  struct event_base *eb_;
  struct evhttp *http_;
  struct evhttp_bound_socket *sock_;

  std::vector<std::unique_ptr<HandleCtx>> handle_ctx_;
};

} // namespace base
