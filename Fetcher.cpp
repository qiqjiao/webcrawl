#include "Fetcher.h"

#include <unistd.h>
#include <string.h>

#include <fstream>
#include <future>
#include <memory>

#include <glog/logging.h>

#include "base/Utils.h"

namespace crawl {

namespace {

struct FetcherContext: public CrawlContext::SubContext {
  ~FetcherContext() override {
    curl_slist_free_all(req_headers);
    curl_slist_free_all(resolve_list);
  }

  curl_slist *req_headers  = nullptr;
  curl_slist *resolve_list = nullptr;
};

} // namespace

size_t write_header_cb(char *buffer, size_t size, size_t nitems, void *userdata) {
  CrawlContext *ctx = static_cast<CrawlContext*>(userdata);
  ctx->resp_headers.push_back(std::string(buffer, size * nitems));
  return size * nitems;
}

size_t write_body_cb(char *ptr, size_t size, size_t nmemb, void *userp) {
  CrawlContext *ctx = static_cast<CrawlContext*>(userp);
  if (ctx->resp_body.size() > Fetcher::kMaxBodySize) {
    ctx->truncated = true;
  } else {
    ctx->resp_body.append(ptr, size * nmemb);
  }
  return size * nmemb;
}

Fetcher::Fetcher() {
  curl_global_init(CURL_GLOBAL_ALL);

  multi_ = curl_multi_init();
  curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, &Fetcher::multi_socket_cb);
  curl_multi_setopt(multi_, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, &Fetcher::multi_timer_cb);
  curl_multi_setopt(multi_, CURLMOPT_TIMERDATA, this);

  stats_timer_handle_ = es_.add_timer([this](){}, base::EventServer::PERSIST);
  es_.start(stats_timer_handle_, 1000);

  thread_ = std::thread([this]() { this->es_.loop(); });

  LOG(INFO) << "Fetcher [" << this << "] started";
}

Fetcher::~Fetcher() {
  es_.exit();
  thread_.join();

  curl_multi_cleanup(multi_);
  curl_global_cleanup();
  LOG(INFO) << "Fetcher [" << this << "] exited";
}

void Fetcher::add(std::shared_ptr<CrawlContext> ctx) {
  auto ares_cb = [this, ctx](const base::Ares::AddrList &addrs, const base::Status &s)
  {
    VLOG(1) << "Fetcher dns resolution done, " << s;
    ctx->end_resolve_time_ms = base::now_in_ms();
    if (!s) {
      ctx->done = true;
      ctx->end_req_time_ms = base::now_in_ms();
      ctx->error_message = s.msg();
      return;
    }
    ctx->ip = addrs[0];

    es_.add_async([this, ctx]() {
      CURL *easy = curl_easy_init();

      curl_easy_setopt(easy, CURLOPT_URL, ctx->uri.str.c_str());
      curl_easy_setopt(easy, CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:43.0) Gecko/20100101 Firefox/43.0"); 
      curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "gzip, deflate"); 
      curl_easy_setopt(easy, CURLOPT_HTTP_CONTENT_DECODING, 1L);

      std::unique_ptr<FetcherContext> fc(new FetcherContext);
      fc->req_headers = curl_slist_append(fc->req_headers, "Accept-Language: en-US,en;q=0.5");
      for (auto const &hdr: ctx->req_headers) {
        fc->req_headers = curl_slist_append(fc->req_headers, hdr.c_str());
      }
      curl_easy_setopt(easy, CURLOPT_HTTPHEADER, fc->req_headers);

      curl_easy_setopt(easy, CURLOPT_PRIVATE, ctx.get());
      curl_easy_setopt(easy, CURLOPT_TIMEOUT, 90L); 

      curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, write_header_cb); 
      curl_easy_setopt(easy, CURLOPT_HEADERDATA, ctx.get());
      curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_body_cb);
      curl_easy_setopt(easy, CURLOPT_WRITEDATA, ctx.get());

      curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 3L);

      //curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
      //curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
      //curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
      //curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);

      std::string http_resolv  = ctx->uri.host + ":80:"  + ctx->ip;
      std::string https_resolv = ctx->uri.host + ":443:" + ctx->ip;
      fc->resolve_list = curl_slist_append(fc->resolve_list, http_resolv.c_str());
      fc->resolve_list = curl_slist_append(fc->resolve_list, https_resolv.c_str());
      curl_easy_setopt(easy, CURLOPT_RESOLVE, fc->resolve_list);
 
      ctx->sub_ctx[typeid(this).name()] = std::move(fc);

      VLOG(1) << "==> " << ctx->method << ' ' << ctx->uri.str;
      curl_multi_add_handle(this->multi_, easy);

      tasks_.push_back(ctx);
    });
  };

  ctx->start_req_time_ms = base::now_in_ms();
  ares_.resolve(ctx->uri.host, ares_cb);
}

Json::Value Fetcher::create_tasks_req(const Json::Value &req_body) {
  Json::Value resp(Json::objectValue);

  try {
    if (tasks_.size() + req_body["tasks"].size() > kMaxTasksCnt) {
      throw std::length_error("Too many pending tasks");
    }

    std::vector<std::shared_ptr<CrawlContext>> ctxs;
    for (auto const& task: req_body["tasks"]) {
      auto ctx = std::make_shared<CrawlContext>();
      ctx->id = reinterpret_cast<long>(ctx.get());
      ctx->method = task["method"].asString();
      ctx->uri.Init(task["uri"].asString());
      for (auto const& hdr: task["headers"]) {
        ctx->req_headers.push_back(hdr.asString());
      }
      ctxs.push_back(ctx);
    }

    resp["tasks"] = Json::Value(Json::arrayValue);
    for (auto ctx: ctxs) {
      Json::Value task(Json::objectValue);
      task["id"] = Json::Int64(ctx->id);
      task["summary"] = ctx->method + " " + ctx->uri.str;
      resp["tasks"].append(task);

      add(ctx);
    }
  } catch (const std::exception &e) {
    resp.clear();
    resp["error"] = e.what();
  }

  return resp;
}

Json::Value Fetcher::get_done_tasks_req() {
  std::promise<Json::Value> promise;
  std::future<Json::Value> future = promise.get_future();

  es_.add_async([this, &promise]() {
    Json::Value resp(Json::objectValue);
    resp["tasks"] = Json::Value(Json::arrayValue);
    std::vector<std::shared_ptr<CrawlContext>> busy_tasks;
    while (!tasks_.empty()) {
      auto ctx = tasks_.back();
      tasks_.pop_back();
      if (ctx->done) {
        Json::Value r(Json::objectValue);
        r["id"] = Json::Int64(ctx->id);
        r["method"] = ctx->method;
        r["uri"] = ctx->uri.str;
        if (!ctx->error_message.empty()) {
          r["error"] = ctx->error_message;
        }
        r["response_code"] = Json::Int64(ctx->resp_code);
        r["response_headers"] = Json::Value(Json::arrayValue);
        for (auto const& h: ctx->resp_headers) {
          r["response_headers"].append(h);
        }
        r["response_body"] = ctx->resp_body;
        resp["tasks"].append(r);
      } else {
        busy_tasks.push_back(ctx);
      }
    }
    tasks_.swap(busy_tasks);
    promise.set_value(std::move(resp));
  });

  return future.get();
}

int Fetcher::socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp) {
  VLOG(3) << "socket cb: " << s << "," << what << "," << socketp;

  struct SocketCtx {
    const base::EventServer::Handle *read  = nullptr;
    const base::EventServer::Handle *write = nullptr;
  };

  SocketCtx *sock_ctx = static_cast<SocketCtx*>(socketp);
  if (sock_ctx == nullptr) {
    sock_ctx = new SocketCtx;

    auto event_cb = [this, easy](int s, int events) {
      int what = 0;
      if (events & base::EventServer::READ)  { what |= CURL_POLL_IN;  }
      if (events & base::EventServer::WRITE) { what |= CURL_POLL_OUT; }
    
      int still_running;
      Fetcher* f = this;
      curl_multi_socket_action(this->multi_, s, what, &still_running);
      VLOG(3) << "read/write cb:" << s << "," << events << ", " << still_running;
      f->check_multi_info();
      if (still_running == 0 && multi_timer_handle_ != nullptr) {
        es_.stop(multi_timer_handle_);
      }
    };
 
    sock_ctx->read  = es_.add_fd(s, base::EventServer::READ |base::EventServer::PERSIST, event_cb);
    sock_ctx->write = es_.add_fd(s, base::EventServer::WRITE|base::EventServer::PERSIST, event_cb);
    curl_multi_assign(multi_, s, sock_ctx);
  }

  switch (what) {
    case CURL_POLL_IN:
      es_.start(sock_ctx->read);
      es_.stop(sock_ctx->write);
      break;

    case CURL_POLL_OUT:
      es_.start(sock_ctx->write);
      es_.stop(sock_ctx->read);
      break;

    case CURL_POLL_INOUT:
      es_.start(sock_ctx->read);
      es_.start(sock_ctx->write);
      break;

    case CURL_POLL_REMOVE:
      es_.remove(sock_ctx->read);
      es_.remove(sock_ctx->write);
      delete sock_ctx;
      curl_multi_assign(multi_, s, nullptr);
      break;

    default: CHECK(0);
  }

  return 0;
}

int Fetcher::timer_cb(CURLM *multi, long timeout_ms) {
  VLOG(3) << "timer cb: " << timeout_ms;

  if (timeout_ms >= 0) {
    if (multi_timer_handle_ == nullptr) {
      multi_timer_handle_ = es_.add_timer([this]() {
        int still_running;
        curl_multi_socket_action(multi_, CURL_SOCKET_TIMEOUT, 0, &still_running);
        this->check_multi_info();
        VLOG(3) << "timer async wait: " << still_running;
      });
    }
    es_.start(multi_timer_handle_, timeout_ms);
  }
  return 0;
}

void Fetcher::check_multi_info() {
  static int cnt = 0;
  CURLMsg *msg;
  int msgs_left = 0;
  VLOG(2) << "Checking multi info";
  while ((msg = curl_multi_info_read(multi_, &msgs_left)))
  {
    CURL *easy = msg->easy_handle;
    CrawlContext *ctx = nullptr;
    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ctx);
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &ctx->resp_code);

    VLOG(2) << "check_multi_info:" << ctx->uri;

    if(msg->msg == CURLMSG_DONE) {
      VLOG(1) << "<== " << ctx->resp_code << ' ' << msg->data.result << ' '
              << ctx->resp_body.size() << ' ' << ++cnt << " ["
              << ctx->method << ' ' << ctx->uri << "]";
      if (msg->data.result != 0) {
        ctx->error_message = curl_easy_strerror(msg->data.result);
        VLOG(2) << msg->data.result << ":" << curl_easy_strerror(msg->data.result);
      }
      ctx->done = true;
      ctx->end_req_time_ms = base::now_in_ms();
      curl_multi_remove_handle(multi_, easy);
      curl_easy_cleanup(easy);
    }
  }
}

int Fetcher::multi_socket_cb(CURL *easy, curl_socket_t s, int what, void *userp,
                    void *socketp) {
  return static_cast<Fetcher*>(userp)->socket_cb(easy, s, what, socketp);
}

int Fetcher::multi_timer_cb(CURLM *multi, long timeout_ms, void *userp) {
  return static_cast<Fetcher*>(userp)->timer_cb(multi, timeout_ms);
}


} // namespace crawl
