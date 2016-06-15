cmake_minimum_required (VERSION 2.8)

project(WebCrawl)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

configure_file (
  "${PROJECT_SOURCE_DIR}/Config.h.in"
  "${PROJECT_BINARY_DIR}/Config.h"
)

# Build options
add_compile_options(-std=c++11 -g -O2)
 
# Libraries
add_library(webcrawl Common.cpp Uri.cpp HttpServer.cpp EventServer.cpp Fetcher.cpp) # Ares.cpp Fetcher.cpp)

add_library(testmain GTestMain.cpp)

# Unit tests
add_subdirectory(/usr/src/gtest ${CMAKE_CURRENT_BINARY_DIR}/gtest)
add_executable(WebCrawlTest EventServer_test.cpp Uri_test.cpp HttpServer_test.cpp Fetcher_test.cpp)# Ares_test.cpp Fetcher_test.cpp)
target_link_libraries(WebCrawlTest webcrawl testmain curl boost_system gflags glog gtest event event_pthreads uv)

# Expriment
#add_executable(Experiment Experiment.cpp)
#target_link_libraries(Experiment event uv)
