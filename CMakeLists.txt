cmake_minimum_required(VERSION 3.0)
project(sylar) 

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/yaml-cpp)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)     #在make过程中显示详细命令
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.) #路径
include_directories(/Downloads/yaml/include)
link_directories(/Downloads/yaml/src)

find_library(YAMLCPP libyaml-cpp.a)

option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    sylar/address.cpp
    sylar/bytearray.cpp
    sylar/config.cpp 
    sylar/daemon.cpp
    sylar/env.cpp
    sylar/fd_manager.cpp   
    sylar/fiber.cpp
    sylar/hook.cpp
    sylar/http/http.cpp
    sylar/http/http_connection.cpp
    sylar/http/http_parser.cpp
    sylar/http/http_session.cpp
    sylar/http/http_server.cpp
    sylar/http/servlet.cpp
    #sylar/http/http11_parser.rl.cpp
    #sylar/http/httpclient_parser.rl.cpp
    sylar/log.cpp
    sylar/socket.cpp
    sylar/socket_stream.cpp
    sylar/stream.cpp
    sylar/application.cpp
    sylar/tcp_server.cpp
    sylar/thread.cpp
    sylar/util.cpp
    sylar/scheduler.cpp
    sylar/iomanager.cpp
    sylar/timer.cpp
    ) #把源码设置到LIB_SRC上

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPETIES {sylar_static PROPERTIES OUTPUT_NAME "sylar"}

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
    sylar
    dl
    pthread
    yaml-cpp
    ${YAMLCPP})

message("***", ${LIBS})


sylar_add_executable(test "tests/test.cpp" sylar "${LIBS}")
sylar_add_executable(test_config "tests/test_config.cpp" sylar "${LIBS}")
sylar_add_executable(test_thread "tests/test_thread.cpp" sylar "${LIBS}")
sylar_add_executable(test_util "tests/test_util.cpp" sylar "${LIBS}")
sylar_add_executable(test_fiber "tests/test_fiber.cpp" sylar "${LIBS}")
sylar_add_executable(test_scheduler "tests/test_scheduler.cpp" sylar "${LIBS}")
sylar_add_executable(test_iomanager "tests/test_iomanager.cpp" sylar "${LIBS}")
sylar_add_executable(test_hook "tests/test_hook.cpp" sylar "${LIBS}")
sylar_add_executable(test_address "tests/test_address.cpp" sylar "${LIBS}")
sylar_add_executable(test_socket "tests/test_socket.cpp" sylar "${LIBS}")
sylar_add_executable(test_bytearray "tests/test_bytearray.cpp" sylar "${LIBS}")
sylar_add_executable(test_http "tests/test_http.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_parser "tests/test_http_parser.cpp" sylar "${LIBS}")
sylar_add_executable(test_tcp_server "tests/test_tcp_server.cpp" sylar "${LIBS}")
sylar_add_executable(echo_server "examples/echo_server.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_server "tests/test_http_server.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_connection "tests/test_http_connection.cpp" sylar "${LIBS}")
sylar_add_executable(test_uri "tests/test_uri.cpp" sylar "${LIBS}")
sylar_add_executable(test_daemon "tests/test_daemon.cpp" sylar "${LIBS}")
sylar_add_executable(test_env "tests/test_env.cpp" sylar "${LIBS}")
sylar_add_executable(my_http_server "samples/my_http_server.cpp" sylar "${LIBS}")
sylar_add_executable(test_application "tests/test_application.cpp" sylar "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)