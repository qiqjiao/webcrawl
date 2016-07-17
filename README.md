* gtest
- apt-get install libgtest-dev
- Only src are installed, no pre-built library
- add_subdirectory(/usr/src/gtest ${CMAKE_CURRENT_BINARY_DIR}/gtest)
  target_link_libraries(<target> gtest gtest_main)

Fetcher -> Analyzer -> Scheduler

* packages
- apt-get install cmake libgoogle-glog-dev libgflags-dev libgtest-dev \
                  libevent-dev libcurl4-openssl-dev \
                  libjsoncpp-dev libcpprest-dev
