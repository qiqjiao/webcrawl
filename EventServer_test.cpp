#include "EventServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <glog/logging.h>

namespace base { namespace test {

class EventServerTest : public testing::Test {
};

TEST_F(EventServerTest, async) {
  EventServer es;

  int i = 0;
  auto cb = [&es, &i]() { ++i; };
  es.add_async(cb);
  es.loop();
  EXPECT_EQ(1, i);
}

TEST_F(EventServerTest, timer) {
  auto start = std::chrono::system_clock::now();

  int i = 0;
  EventServer es;
  auto h = es.add_timer([&es, &i]() { if (++i == 75) es.exit(); },
                        EventServer::PERSIST);
  es.start(h, 10);
  es.loop();

  EXPECT_EQ(75, i);

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  EXPECT_TRUE(0.5 < elapsed.count() && elapsed.count() < 1.0) << elapsed.count();
}

TEST_F(EventServerTest, stop) {
  EventServer es;
  bool b1 = false;
  auto h1 = es.add_timer([&b1]() { b1 = true; });
  auto h2 = es.add_timer([&es, h1]() { es.stop(h1); });
  es.start(h1, 20);
  es.start(h2, 10);
  es.loop();
}

TEST_F(EventServerTest, fd) {
  int port = 50000 + time(nullptr) % 15536;
  int MSG_SZ = 1024*1024*16;

  auto svr = std::thread([port, MSG_SZ]() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(-1, sock);

    struct sockaddr_in svr_addr, client_addr;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    svr_addr.sin_port = port;

    ASSERT_EQ(0, bind(sock, (sockaddr*)&svr_addr, sizeof(svr_addr)));
    ASSERT_EQ(0, listen(sock, 1));

    socklen_t addr_len = sizeof(sockaddr_in);
    int fd = accept(sock, (sockaddr*)&client_addr, &addr_len);

    std::string r;
    while (r.size() < MSG_SZ) {
      char buffer[1024];
      int c = read(fd, buffer, sizeof(buffer));
      if (c > 0) { r.append(buffer, c); }
      VLOG(1) << "Server received " << r.size();
    }

    int sent = 0;
    while (sent < r.size()) {
      int c = write(fd, &r[sent], r.size() - sent);
      if (c > 0) { sent += c; }
      VLOG(1) << "Server wrote " << sent;
    }

    close(fd);
    close(sock);
    VLOG(1) << "Server sockets are closed";
  });
  usleep(100*1000);
  
  int sock = socket(AF_INET, SOCK_STREAM, 0); 
  ASSERT_NE(-1, sock);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = port;

  ASSERT_EQ(0, connect(sock, (sockaddr*)&addr, sizeof(addr)));

  int sent= 0;
  std::string s(MSG_SZ, 's'), r;

  EventServer es;
  auto cb = [&es, &s, &r, &sent](int sock, int events) {
    VLOG(2) << "CALLBACK " << sock << " " << events;
    if (events & EventServer::READ) {
      char buffer[1024];
      int c = 0;
      while ((c = read(sock, buffer, sizeof(buffer))) > 0) {
        r.append(buffer, c);
        VLOG(1) << "Client received " << r.size();
      }
    }

    if (events & EventServer::WRITE) {
      if (sent < s.size()) {
        int c = 0;
        while ((c = write(sock, &s[sent], s.size() - sent)) > 0) {
          sent += c;
          VLOG(1) << "Client wrote " << sent;
        }
      }
    }
  };

  int wait = 0;
  auto timer_h = es.add_timer([&es, &r, MSG_SZ, &wait, sock]() {
    if (r.size() == MSG_SZ || wait++ == 100 /* 10s */) {
      es.exit();
    }
  }, EventServer::PERSIST);
  es.start(timer_h, 100); 

  int flags = EventServer::READ|EventServer::WRITE|EventServer::PERSIST;
  auto fd_h = es.add_fd(sock, flags, cb);
  es.start(fd_h);
  es.loop();

  EXPECT_EQ(s.size(), sent);
  EXPECT_TRUE(s == r);

  svr.join();
}

}} // namespace base::test
