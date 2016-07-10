cmake_minimum_required (VERSION 2.8)

project(WebCrawl)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

add_subdirectory(3rd)

enable_testing()

include_directories(/usr/local/include)
include_directories(/usr/include/jsoncpp)
include_directories(3rd/googletest-1.7.0/include)
link_directories(/usr/local/lib)

configure_file (
  "${PROJECT_SOURCE_DIR}/Config.h.in"
  "${PROJECT_BINARY_DIR}/Config.h"
)

# Build options
add_compile_options(-std=c++1y -g -O2)
 
# base
add_subdirectory(base)

# Libraries
add_library(webcrawl CrawlContext.cpp Fetcher.cpp)
target_link_libraries(webcrawl base curl)

# Unit tests
add_executable(WebCrawlTest Fetcher_test.cpp)
target_link_libraries(WebCrawlTest base webcrawl testmain)
add_test(WebCrawlTest WebCrawlTest)

# Expriment
#add_executable(Experiment Experiment.cpp)
#target_link_libraries(Experiment event uv)

# Fetcher Server
add_executable(FetcherServer FetcherServer.cpp)
target_link_libraries(FetcherServer base webcrawl)
