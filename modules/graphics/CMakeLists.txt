cmake_minimum_required(VERSION 2.8)
project(fuzzy.graphics)

find_package(OpenGL REQUIRED)

set(root_dir ../..)
set(build_dir build)

add_subdirectory(${root_dir}/externals/glfw ${build_dir}/glfw)
add_subdirectory(${root_dir}/externals/glew ${build_dir}/glew)

add_library(glew STATIC ${root_dir}/externals/glew/src/glew.c)

include_directories(${root_dir}/externals/glfw/include)
include_directories(${root_dir}/externals/glew/include)
include_directories(${root_dir}/externals/glm)
include_directories(${root_dir}/externals/stb)

set(sources include/Window.h src/Window.cpp include/WindowManager.h src/WindowManager.cpp include/glutils/Shader.h include/glutils/ShaderProgram.h src/glutils/Shader.cpp src/glutils/ShaderProgram.cpp include/Texture.h src/Texture.cpp)

add_library(fuzzy.graphics SHARED ${sources})

target_link_libraries(fuzzy.graphics
        glfw glew ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY})

add_definitions(-DGLEW_STATIC)
#add_definitions(-DSTB_IMAGE_IMPLEMENTATION)