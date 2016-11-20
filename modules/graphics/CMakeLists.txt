cmake_minimum_required(VERSION 3.0)
project(fuzzy.graphics)

find_package(OpenGL REQUIRED)

set(root_dir ../..)
set(build_dir build)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${root_dir}/externals/glfw ${build_dir}/glfw)
add_subdirectory(${root_dir}/externals/glew ${build_dir}/glew)
add_subdirectory(${root_dir}/externals/assimp ${build_dir}/assimp)

add_library(glew STATIC ${root_dir}/externals/glew/src/glew.c)

include_directories(${root_dir}/externals/glfw/include)
include_directories(${root_dir}/externals/glew/include)
include_directories(${root_dir}/externals/glm)
include_directories(${root_dir}/externals/stb)
include_directories(${root_dir}/externals/assimp/include)

set(sources include/Window.h src/Window.cpp include/WindowManager.h src/WindowManager.cpp include/glutils/Shader.h include/glutils/ShaderProgram.h src/glutils/Shader.cpp src/glutils/ShaderProgram.cpp include/Texture.h src/Texture.cpp include/Camera.h src/Camera.cpp)

add_library(fuzzy.graphics STATIC ${sources})

target_link_libraries(fuzzy.graphics
        assimp glfw glew ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY})

add_definitions(-DGLEW_STATIC)