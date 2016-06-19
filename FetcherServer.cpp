#include <sstream>

#include <json/json.h>
#include <gflags/gflags.h>

#include "base/HttpServer.h"
#include "Fetcher.h"


DEFINE_int32(fetcher_rest_svr_port, 31200, "Fetcher rest server port");

using namespace base;
using namespace crawl;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, false);                                                 

  Fetcher fetcher;

  HttpServer svr("0.0.0.0", FLAGS_fetcher_rest_svr_port);

  svr.AddHandle("/fetch", [&fetcher](const HttpServer::Request &req, HttpServer::Response *resp) {
    Json::Value r;
    Json::Reader reader;
    if (!reader.parse(req.body, r)) {
      resp->code = 400;
      resp->reason = "Bad Request";
      resp->body = reader.getFormattedErrorMessages();
      return;
    }

    Json::Value resp_body = fetcher.create_tasks_req(r);

    if (resp_body.isMember("error")) {
      resp->code = 400;
      resp->reason = "Bad Request";
    } else {
      resp->code = 201;
      resp->reason = "Created";
    }

    Json::StyledWriter writer;
    resp->body = writer.write(resp_body);
  });

  svr.AddHandle("/result", [&fetcher](const HttpServer::Request &req, HttpServer::Response *resp) {
    Json::Value r = fetcher.get_done_tasks_req();

    Json::StyledWriter writer;
    resp->code = 200;
    resp->reason = "OK";
    resp->body = writer.write(r);
  });

  return svr.Run() ? 0 : 1;
}
