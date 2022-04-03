cmake_minimum_required(VERSION 3.21)
project(online_chat)

set(CMAKE_CXX_STANDARD 14)

add_executable(online_chat server.cpp client.cpp)
