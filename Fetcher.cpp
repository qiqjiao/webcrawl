#include "Fetcher.h"

#include <unistd.h>
#include <string.h>

#include <fstream>

#include <glog/logging.h>

namespace crawl {

size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userp) {
  CrawlContext *ctx = static_cast<CrawlContext*>(userp);
  ctx->response_page.append(ptr, size * nmemb);
  VLOG(3) << "write cb: " << ctx->uri << "," << size * nmemb;
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
}

Fetcher::~Fetcher() {
  es_.exit();
  thread_.join();

  curl_multi_cleanup(multi_);
  curl_global_cleanup();
}

bool Fetcher::add(std::shared_ptr<CrawlContext> ctx) {
  es_.add_async([this, ctx]() {
    CURL *easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, ctx->uri.uri.c_str());

    curl_easy_setopt(easy, CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:43.0) Gecko/20100101 Firefox/43.0"); 
    //curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "gzip, deflate"); 

    curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.5");
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

    //FILE *file = fopen("/dev/null", "wb");
    //curl_easy_setopt(easy, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, ctx.get());
    curl_easy_setopt(easy, CURLOPT_PRIVATE, ctx.get());
    //curl_easy_setopt(easy, CURLOPT_PRIVATE, ctx->uri.uri.c_str());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 90L); 

    //curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);

    //curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);
    //std::string http_resolv  = ctx->url.hostname + ":80:"  + ctx->ip_str;
    //std::string https_resolv = ctx->url.hostname + ":443:" + ctx->ip_str;
    //curl_slist *resolve_lsit = NULL;
    //resolve_lsit = curl_slist_append(resolve_lsit, http_resolv.c_str());
    //resolve_lsit = curl_slist_append(resolve_lsit, https_resolv.c_str());
    //curl_easy_setopt(easy, CURLOPT_RESOLVE, resolve_lsit);
 
    VLOG(1) << "Adding url: " << ctx->uri.uri;
    curl_multi_add_handle(this->multi_, easy);

    tasks_.push_back(ctx);
  });

  return true;
}

int Fetcher::socket_cb(CURL *easy, curl_socket_t s, int what, void *socketp) {
  VLOG(1) << "socket cb: " << s << "," << what << "," << socketp;

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
      VLOG(1) << "read/write cb:" << s << "," << events << ", " << still_running;
      f->check_multi_info();
      if (still_running == 0 && multi_timer_handle_ != nullptr) {
        es_.stop(multi_timer_handle_);
      }
    };
 
    sock_ctx->read  = es_.add_fd(s, base::EventServer::READ |base::EventServer::PERSIST, event_cb);
    sock_ctx->write = es_.add_fd(s, base::EventServer::WRITE|base::EventServer::PERSIST, event_cb);
    //sock_ctx->read  = es_.add_fd(s, EventServer::READ , event_cb);
    //sock_ctx->write = es_.add_fd(s, EventServer::WRITE, event_cb);
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
      VLOG(1) << "Remove socket:" << s;
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
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &ctx->response_code);

    VLOG(2) << "check_multi_info:" << ctx->uri;

    if(msg->msg == CURLMSG_DONE) {
      LOG(INFO) << "DONE: " << ctx->uri << ", code: " << ctx->response_code
              << " curlcode:" << msg->data.result << ", curlmsg:" << curl_easy_strerror(msg->data.result)
              << ", page: " << ctx->response_page.size() << ", cnt=" << ++cnt;
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
