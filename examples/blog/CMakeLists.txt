cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0015 NEW)

project(wayward_examples_blog)

include_directories(../..)
include_directories(/usr/local/include)
set(CMAKE_CXX_FLAGS "-std=c++11 -O0 -g")

add_executable(blog
  blog.cpp
)

add_subdirectory("../.." wayward.bin)
target_link_libraries(blog wayward persistence)
