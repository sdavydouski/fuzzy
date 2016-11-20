cmake_minimum_required(VERSION 3.0)
project(fuzzy.game)

include_directories(../modules/graphics/include)
include_directories(../externals/glew/include)
include_directories(../externals/glm)
include_directories(../externals/stb)

set(sources src/main.cpp)

add_executable(fuzzy.game ${sources})

target_link_libraries(fuzzy.game fuzzy.graphics)

add_definitions(-DGLEW_STATIC)