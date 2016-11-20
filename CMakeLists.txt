cmake_minimum_required(VERSION 3.0)
project(fuzzy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2")

add_subdirectory(game)
add_subdirectory(modules/graphics)