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

// http://www.wangafu.net/~nickm/libevent-2.0/doxygen/html/http_8h.html#a5404c30f3b50a664f2ec1500ebb30d86
// https://github.com/libevent/libevent/blob/master/sample/http-server.c
// https://github.com/libevent/libevent/blob/master/sample/https-client.c
// https://github.com/libevent/libevent/blob/master/http.c
// http://blog.csdn.net/pcliuguangtao/article/details/9360331
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
