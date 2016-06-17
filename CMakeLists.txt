cmake_minimum_required (VERSION 2.8)

project(WebCrawl)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

configure_file (
  "${PROJECT_SOURCE_DIR}/Config.h.in"
  "${PROJECT_BINARY_DIR}/Config.h"
)

# Build options
add_compile_options(-std=c++14 -g -O2)
 
# GTest
add_subdirectory(/usr/src/gtest ${CMAKE_CURRENT_BINARY_DIR}/gtest)

# Libraries
add_library(base Utils.cpp Uri.cpp EventServer.cpp HttpServer.cpp Ares.cpp)
target_link_libraries(base gflags glog event event_pthreads jsoncpp cares)
add_library(webcrawl Fetcher.cpp)
target_link_libraries(webcrawl base curl)
add_library(testmain GTestMain.cpp)
target_link_libraries(testmain gtest)

# Unit tests
add_executable(BaseTest Uri_test.cpp EventServer_test.cpp HttpServer_test.cpp Ares_test.cpp)
target_link_libraries(BaseTest base testmain gflags glog gtest event event_pthreads jsoncpp)

add_executable(WebCrawlTest EventServer_test.cpp Uri_test.cpp HttpServer_test.cpp Fetcher_test.cpp)
target_link_libraries(WebCrawlTest base webcrawl testmain curl gflags glog gtest event event_pthreads jsoncpp)

# Expriment
#add_executable(Experiment Experiment.cpp)
#target_link_libraries(Experiment event uv)

# Fetcher Server
add_executable(FetcherServer FetcherServer.cpp)
target_link_libraries(FetcherServer base webcrawl curl boost_system gflags glog event event_pthreads jsoncpp pthread)
