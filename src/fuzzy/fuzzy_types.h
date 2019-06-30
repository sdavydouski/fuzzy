#pragma once

#include <stdint.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define global static
#define internal static
#define persist static

// todo: get rid of crt version
#define Assert assert

#define InvalidCodePath assert(!"InvalidCodePath")

#define U32Max UINT32_MAX;

#pragma region GLM types
using glm::translate;
using glm::scale;
using glm::rotate;
using glm::ortho;
using glm::radians;

using vec2 = glm::vec2;
using ivec2 = glm::ivec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat2 = glm::mat2;
using mat4 = glm::mat4;
#pragma endregion

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using b32 = s32;

using f32 = float;
using f64 = double;

using wchar = wchar_t;

using memory_index = size_t;
