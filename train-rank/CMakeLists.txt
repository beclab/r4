cmake_minimum_required(VERSION 3.18)
project(ranktrain)

# find_package(libmongocxx REQUIRED)
# find_package(libbsoncxx REQUIRED)
# include_directories(${LIBMONGOCXX_INCLUDE_DIR})
# include_directories(${LIBBSONCXX_INCLUDE_DIR})
# include_directories("/usr/local/include/mongocxx/v_noabi")
# include_directories("/usr/local/include/bsoncxx/v_noabi")
# include_directories("/usr/local/include/libmongoc-1.0")
# include_directories("/usr/local/include/libbson-1.0")
# https://stackoverflow.com/questions/37210716/using-the-mongodb-cxx-driver-in-a-cmake-c-project
# include_directories("/usr/local/lib")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(rankcommon STATIC src/data_process.cpp
                              src/easylogging++.h
                              src/easylogging++.cc
                              src/common_tool.h
                              src/common_tool.cpp
                              src/rssrank.h
                              src/rssrank.cpp
                              src/knowledgebase_api.h
                              src/knowledgebase_api.cpp)
                              


                              
find_package(xgboost REQUIRED)
find_package(cpprestsdk REQUIRED)



enable_testing()

add_executable(
  hello_test
  test/hello_test.cpp
)
target_link_libraries(
  hello_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
)

link_directories("/usr/lib")
add_executable(
  data_process_test
  test/data_process_test.cpp 
)

target_link_libraries(
  data_process_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  
)

add_executable(
  bertv2_test
  test/bert_v2_test.cpp
  test/test_common.h
 )



 target_link_libraries(
  bertv2_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  xgboost::xgboost 
  #${LIBMONGOCXX_LIBRARIES}
#${LIBBSONCXX_LIBRARIES}
cpprestsdk::cpprest
 boost_system
 crypto
 ssl
 )



 add_executable( 
  bertv2train
  src/bert_v2_train_executable.cpp
)

target_link_libraries(
  bertv2train
  rankcommon
  xgboost::xgboost 
  # ${LIBMONGOCXX_LIBRARIES}
# ${LIBBSONCXX_LIBRARIES}
cpprestsdk::cpprest
 boost_system
 crypto
 ssl
 )

 add_executable( 
  bertv2rank
  src/bert_v2_rank.cpp
)

target_link_libraries(
  bertv2rank
  rankcommon
  xgboost::xgboost 
  # ${LIBMONGOCXX_LIBRARIES}
# ${LIBBSONCXX_LIBRARIES}
cpprestsdk::cpprest
 boost_system
 crypto
 ssl
 )



 add_executable(
  common_test
  test/common_test.cpp
 )
 target_link_libraries(
  common_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
)

add_executable(
  knowledge_api_test
  test/knowledge_api_test.cpp
  test/test_common.h
)

target_link_libraries(
  knowledge_api_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  xgboost::xgboost 
 # ${LIBMONGOCXX_LIBRARIES}
#${LIBBSONCXX_LIBRARIES}
cpprestsdk::cpprest
 boost_system
 crypto
 ssl
 )

 add_executable(
  test_cpprest
  test/cpprest_test.cpp
  )

  target_link_libraries(
    test_cpprest
    cpprestsdk::cpprest
    crypto
    ssl
  )

# https://github.com/Microsoft/cpprestsdk/wiki/How-to-build-for-Linux

include(GoogleTest)
gtest_discover_tests(hello_test)
gtest_discover_tests(data_process_test)

