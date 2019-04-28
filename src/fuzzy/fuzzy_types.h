#pragma once

#include <stdint.h>
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#define global_variable static
#define internal_function static
#define local_persist static

#define InvalidCodePath assert(!"InvalidCodePath")

#define U32Max UINT32_MAX;

#pragma region GLM types
using vec2 = glm::vec2;
using ivec2 = glm::ivec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat2 = glm::mat2;
using mat4 = glm::mat4;
#pragma endregion

#pragma region OpenGL types
using s8 = GLbyte;
using s16 = GLshort;
using s32 = GLint;
using s64 = GLint64;

using u8 = GLubyte;
using u16 = GLushort;
using u32 = GLuint;
using u64 = GLuint64;

using b32 = s32;

using f32 = GLfloat;
using f64 = GLdouble;
#pragma endregion

#pragma region Game types
using memory_index = size_t;
#pragma endregion
