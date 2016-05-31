#include <gflags/gflags.h>                                                                            
#include <glog/logging.h>                                                                             
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  //google::InstallFailureSignalHandler();                                                              
  testing::InitGoogleTest(&argc, argv);                                                               
  google::ParseCommandLineFlags(&argc, &argv, false);                                                 
                                                                                                      
  //// Absorb all the memory leaks from glog vlog here so we only need to deal                          
  //// with it once in valgrind.                                                                        
  //#define INITVLOG(x) VLOG(x) << "INIT VLOG " #x;                                                     
  //INITVLOG(1);                                                                                        
  //INITVLOG(2);                                                                                        
  //INITVLOG(3);                                                                                        
  //INITVLOG(4);                                                                                        
  //INITVLOG(5);                                                                                        
  //INITVLOG(6);                                                                                        
  //INITVLOG(7);                                                                                        
  //INITVLOG(8);                                                                                        
                                                                                                      
  int ret = RUN_ALL_TESTS();                                                                          
                                                                                                      
  //google::protobuf::ShutdownProtobufLibrary();                                                        
  google::ShutDownCommandLineFlags();                                                                 
                                                                                                      
  return ret;                                                                                         

}
