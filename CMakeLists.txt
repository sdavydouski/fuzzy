cmake_minimum_required(VERSION 2.8)
project(fuzzy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(game)
add_subdirectory(modules/graphics)