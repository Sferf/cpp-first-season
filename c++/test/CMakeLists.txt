
cmake_minimum_required(VERSION 3.22)
project(test)
set(CMAKE_CXX_COMPILER "clang++")

add_executable(test deque.h test.cpp DequeTests.hpp DequeTests.cpp
  TestLib.hpp)

target_compile_options(test PRIVATE -std=c++20 -Wall -Wextra -Werror -g )
