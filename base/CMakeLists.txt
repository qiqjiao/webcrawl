# Libraries
add_library(base
	    Base64.cpp EventServer.cpp HttpServer.cpp Resolver.cpp
	    Uri.cpp Utils.cpp)
target_link_libraries(base gflags glog event event_pthreads jsoncpp pthread)

# GTest 
add_library(testmain GTestMain.cpp)
target_link_libraries(testmain gtest)

# Unit tests
add_executable(BaseTest
	       Base64_test.cpp HttpServer_test.cpp EventServer_test.cpp
	       Resolver_test.cpp Uri_test.cpp)
target_link_libraries(BaseTest base testmain)
add_test(BaseTest BaseTest)
