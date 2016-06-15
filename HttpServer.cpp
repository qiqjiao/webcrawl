#include "HttpServer.h"

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

#include <glog/logging.h>

namespace base {

struct HttpServer::HandleCtx {
  HttpServer::HandleCb handle_cb;
};

HttpServer::HttpServer(const std::string& address, int port)
    : address_(address),
      port_(port)
{
  eb_ = event_base_new();
  http_ = evhttp_new(eb_);
}

HttpServer::~HttpServer() {
  evhttp_free(http_);
  event_base_free(eb_);
}

bool HttpServer::Run() {
  sock_ = evhttp_bind_socket_with_handle(http_, address_.c_str(), port_);
  return event_base_dispatch(eb_) != -1;
}

void HttpServer::Stop() {
  evhttp_del_accept_socket(http_, sock_);
}

void HttpServer::AddHandle(const std::string& path, const HandleCb& cb) {
  void (*evcb)(evhttp_request*, void*) = [](evhttp_request* evreq, void *arg) {
    Request req;
    Response resp;
    HandleCtx *ctx = static_cast<HandleCtx*>(arg);

    switch (evhttp_request_get_command(evreq)) {
      case EVHTTP_REQ_GET:     req.method = "GET";     break;
      case EVHTTP_REQ_POST:    req.method = "POST";    break;
      case EVHTTP_REQ_HEAD:    req.method = "HEAD";    break;
      case EVHTTP_REQ_PUT:     req.method = "PUT";     break;
      case EVHTTP_REQ_DELETE:  req.method = "DELETE";  break;
      case EVHTTP_REQ_OPTIONS: req.method = "OPTIONS"; break;
      case EVHTTP_REQ_TRACE:   req.method = "TRACE";   break;
      case EVHTTP_REQ_CONNECT: req.method = "CONNECT"; break;
      case EVHTTP_REQ_PATCH:   req.method = "PATCH";   break;
      default:                 req.method = "UNKNOWN"; break;
    }
    VLOG(2) << "Request [" << req.method << "]";

    auto set_str = [](std::string *d, const char *s) {
      if (s != nullptr) { d->assign(s); }
    };
    const evhttp_uri *evuri = evhttp_request_get_evhttp_uri(evreq);
    set_str(&req.uri.uri      , evhttp_request_get_uri(evreq));
    set_str(&req.uri.hostname , evhttp_request_get_host(evreq));
    set_str(&req.uri.scheme   , evhttp_uri_get_scheme(evuri));
    set_str(&req.uri.username , evhttp_uri_get_userinfo(evuri));
    set_str(&req.uri.password , evhttp_uri_get_userinfo(evuri));
    set_str(&req.uri.hostname , evhttp_uri_get_host(evuri));
    set_str(&req.uri.path     , evhttp_uri_get_path(evuri));
    set_str(&req.uri.query    , evhttp_uri_get_query(evuri));
    set_str(&req.uri.fragment , evhttp_uri_get_fragment(evuri));
    req.uri.port = evhttp_uri_get_port(evuri);

    evkeyvalq *evheaders = evhttp_request_get_input_headers(evreq);
    for (evkeyval *h = evheaders->tqh_first; h; h = h->next.tqe_next) {
      req.headers[h->key] = h->value;
    }

    evbuffer *in_body = evhttp_request_get_input_buffer(evreq);
    int sz = evbuffer_get_length(in_body);
    req.body.resize(sz);
    evbuffer_remove(in_body, const_cast<char*>(req.body.data()), sz);

    ctx->handle_cb(req, &resp);

    for (const auto& e: resp.headers) {
      evkeyvalq *headers = evhttp_request_get_output_headers(evreq);
      CHECK_NE(evhttp_add_header(headers, e.first.data(), e.second.data()), -1);
    }
    evbuffer *body = evhttp_request_get_output_buffer(evreq);
    CHECK_NE(evbuffer_add(body, resp.body.data(), resp.body.size()), -1);

    evhttp_send_reply(evreq, resp.code, resp.reason.data(), nullptr);
  };

  std::unique_ptr<HandleCtx> ctx(new HandleCtx);

  CHECK_NE(evhttp_set_cb(http_, path.c_str(), evcb, ctx.get()), -1);

  ctx->handle_cb = cb;
  handle_ctx_.push_back(std::move(ctx));
}

} // namespace base
