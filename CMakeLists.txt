cmake_minimum_required(VERSION 3.10)

project(Http)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Http http_server.cpp http_tcpserver.cpp)

target_include_directories(Http PRIVATE .)

 target_link_libraries(Http wsock32)
