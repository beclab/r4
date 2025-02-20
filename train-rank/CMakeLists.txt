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

include_directories("/usr/local/include")
link_directories("/usr/local/lib")


# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build
add_custom_command(
    OUTPUT src/gitinfo.h
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

add_custom_target(
  gen_git_info
  DEPENDS src/gitinfo.h
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(GFLAGS_NAMESPACE "google")
set(_GLIBCXX_USE_CXX11_ABI 0)
add_library(rankcommon STATIC src/data_process.cpp
                              src/easylogging++.h
                              src/easylogging++.cc
                              src/common_tool.h
                              src/common_tool.cpp
                              src/rssrank.h
                              src/rssrank.cpp
                              src/knowledgebase_api.h
                              src/knowledgebase_api.cpp
                              src/lr/logistic_regression.cpp
                              src/lr/feature_extractor.cpp
                              src/lr/model_serializer.cpp
                              src/entity/reco_metadata.cpp
                              src/http_single_client.h
                              src/http_single_client.cpp
                              src/faiss_article_search.h
                              src/faiss_article_search.cpp
                              src/userembedding_calculation.h
                              src/userembedding_calculation.cpp
                              src/dump_traceinfo.h
                              src/dump_traceinfo.cpp
                              src/archive_file.h
                              src/archive_file.cpp
                              src/json.hpp
                              )
                              
add_dependencies(rankcommon gen_git_info)

                              
find_package(xgboost REQUIRED)
find_package(cpprestsdk REQUIRED)
#find_package(gflags REQUIRED)


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
  openblas
  lapack
  faiss
  ssl
  crypto
  xlnt
  archive
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
 gflags
 openblas
 lapack
 faiss
 xlnt
 archive
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
 gflags  
 openblas
 lapack
 faiss
 xlnt
archive
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
  gflags
  openblas
  lapack
  faiss
  xlnt
  archive
)

add_executable(
  embedding_inspector
  src/tools/embedding_inspector.cpp
)

target_link_libraries(
  embedding_inspector
  rankcommon
  xgboost::xgboost
  # ${LIBMONGOCXX_LIBRARIES}
  # ${LIBBSONCXX_LIBRARIES}
  cpprestsdk::cpprest
  boost_system
  ssl
  crypto
  gflags
  openblas
  lapack
  faiss
  xlnt
  archive
)

add_executable(
  experiment_similarity_search
  test/experiment/experiment_similarity_search.cpp
)


 add_executable(
  common_test
  test/common_test.cpp
 )
 target_link_libraries(
  common_test
  openblas
  lapack
  faiss
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  ssl
  crypto
  xlnt
  archive
)

add_executable(
  knowledge_api_test
  test/knowledge_api_test.cpp
  test/test_common.h
  src/rssrank.h
)

target_link_libraries(
  knowledge_api_test
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  xgboost::xgboost 
  openblas
  lapack
  faiss
 # ${LIBMONGOCXX_LIBRARIES}
#${LIBBSONCXX_LIBRARIES}
cpprestsdk::cpprest
 boost_system
 ssl
 crypto
 gflags
 xlnt
 archive
 )
add_executable(
  parameter_tune_tool
  src/recommend_parameter_tune_tool.cpp
)

target_link_libraries(
  parameter_tune_tool
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  xgboost::xgboost 
  openblas
  lapack
  faiss
  # ${LIBMONGOCXX_LIBRARIES}
  # ${LIBBSONCXX_LIBRARIES}
  cpprestsdk::cpprest
  boost_system
  ssl
  crypto
  gflags
  xlnt
  archive
)

add_executable(
  dump_traceinfo_tool
  src/dump_traceinfo_tool.cpp
)

target_link_libraries(
  dump_traceinfo_tool
  libgtest_main.a
  libgmock.a  
  libgmock_main.a  
  libgtest.a
  rankcommon
  xgboost::xgboost 
  openblas
  lapack
  faiss
  # ${LIBMONGOCXX_LIBRARIES}
  # ${LIBBSONCXX_LIBRARIES}
  cpprestsdk::cpprest
  boost_system
  ssl
  crypto
  gflags
  xlnt
  archive
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
    gflags
  )

# https://github.com/Microsoft/cpprestsdk/wiki/How-to-build-for-Linux

include(GoogleTest)
gtest_discover_tests(hello_test)
gtest_discover_tests(data_process_test)

