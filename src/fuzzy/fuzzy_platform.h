#pragma once

#include <string.h>
#include "fuzzy_types.h"

#define EXPORT __declspec(dllexport)

#pragma region Platform API

#define PLATFORM_PRINT_OUTPUT(name) void name(const char *Output)
typedef PLATFORM_PRINT_OUTPUT(platform_print_output);

struct read_file_result
{
    u32 Size;
    void *Contents;
};

#define PLATFORM_READ_FILE(name) read_file_result name(char *FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FREE_FILE(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE(platform_free_file);

#define PLATFORM_READ_IMAGE_FILE(name) u8 *name(const char *Filename, s32 *X, s32 *Y, s32 *Comp, s32 ReqComp)
typedef PLATFORM_READ_IMAGE_FILE(platform_read_image_file);

#define PLATFORM_FREE_IMAGE_FILE(name) void name(void *Image)
typedef PLATFORM_FREE_IMAGE_FILE(platform_free_image_file);

#pragma endregion

struct platform_api {
    platform_print_output *PrintOutput;

    platform_read_file *ReadFile;
    platform_free_file *FreeFile;

    platform_read_image_file *ReadImageFile;
    platform_free_image_file *FreeImageFile;
};

#pragma region Renderer API

#define GL_CREATE_SHADER(name) u32 name(GLenum ShaderType)
typedef GL_CREATE_SHADER(gl_create_shader);

#define GL_SHADER_SOURCE(name) void name(u32 Shader, GLsizei Count, const GLchar *const *String, const s32 *Length)
typedef GL_SHADER_SOURCE(gl_shader_source);

#define GL_COMPILE_SHADER(name) void name(u32 Shader)
typedef GL_COMPILE_SHADER(gl_compile_shader);

#define GL_GET_SHADER_IV(name) void name(u32 Shader, GLenum Pname, s32 *Params)
typedef GL_GET_SHADER_IV(gl_get_shader_iv);

#define GL_GET_SHADER_INFO_LOG(name) void name(u32 Shader, GLsizei MaxLength, GLsizei *Length, GLchar *InfoLog)
typedef GL_GET_SHADER_INFO_LOG(gl_get_shader_info_log);

#define GL_DELETE_SHADER(name) void name(u32 Shader)
typedef GL_DELETE_SHADER(gl_delete_shader);

#define GL_GET_UNIFORM_LOCATION(name) s32 name(u32 Program, const GLchar *Name)
typedef GL_GET_UNIFORM_LOCATION(gl_get_uniform_location);

#define GL_UNIFORM_1I(name) void name(s32 Location, s32 V0)
typedef GL_UNIFORM_1I(gl_uniform_1i);

#define GL_UNIFORM_1F(name) void name(s32 Location, f32 V0)
typedef GL_UNIFORM_1F(gl_uniform_1f);

#define GL_UNIFORM_2F(name) void name(s32 Location, f32 V0, f32 V1)
typedef GL_UNIFORM_2F(gl_uniform_2f);

#define GL_UNIFORM_MATRIX_4FV(name) void name(s32 Location, GLsizei Count, GLboolean Transpose, const f32 *Value)
typedef GL_UNIFORM_MATRIX_4FV(gl_uniform_matrix_4fv);

#define GL_GEN_TEXTURES(name) void name(GLsizei N, u32 *Textures)
typedef GL_GEN_TEXTURES(gl_gen_textures);

#define GL_BIND_TEXTURE(name) void name(GLenum Target, u32 Texture)
typedef GL_BIND_TEXTURE(gl_bind_texture);

#define GL_TEX_PARAMETER_I(name) void name(GLenum Target, GLenum Pname, s32 Param)
typedef GL_TEX_PARAMETER_I(gl_tex_parameter_i);

#define GL_TEX_IMAGE_2D(name) void name(GLenum Target, s32 Level, s32 InternalFormat,\
                                        GLsizei Width, GLsizei Height, s32 Border,\
                                        GLenum Format, GLenum Type, const GLvoid *Data)
typedef GL_TEX_IMAGE_2D(gl_tex_image_2d);

#define GL_CREATE_PROGRAM(name) u32 name(void)
typedef GL_CREATE_PROGRAM(gl_create_program);

#define GL_ATTACH_SHADER(name) void name(u32 Program, u32 Shader)
typedef GL_ATTACH_SHADER(gl_attach_shader);

#define GL_LINK_PROGRAM(name) void name(u32 Program)
typedef GL_LINK_PROGRAM(gl_link_program);

#define GL_GET_PROGRAM_IV(name) void name(u32 Program, GLenum Pname, s32 *Params)
typedef GL_GET_PROGRAM_IV(gl_get_program_iv);

#define GL_GET_PROGRAM_INFO_LOG(name) void name(u32 Program, GLsizei MaxLength, GLsizei *Length, GLchar *InfoLog)
typedef GL_GET_PROGRAM_INFO_LOG(gl_get_program_info_log);

#define GL_USE_PROGRAM(name) void name(u32 Program)
typedef GL_USE_PROGRAM(gl_use_program);

#define GL_GET_VERTEX_ARRAYS(name) void name(GLsizei N, u32 *Arrays)
typedef GL_GET_VERTEX_ARRAYS(gl_get_vertex_arrays);

#define GL_BIND_VERTEX_ARRAY(name) void name(u32 Array)
typedef GL_BIND_VERTEX_ARRAY(gl_bind_vertex_array);

#define GL_GEN_BUFFERS(name) void name(GLsizei N, u32 *Buffers)
typedef GL_GEN_BUFFERS(gl_gen_buffers);

#define GL_BIND_BUFFER(name) void name(GLenum Target, u32 Buffer)
typedef GL_BIND_BUFFER(gl_bind_buffer);

#define GL_BUFFER_DATA(name) void name(GLenum Target, GLsizeiptr Size, const GLvoid *Data, GLenum Usage)
typedef GL_BUFFER_DATA(gl_buffer_data);

#define GL_BIND_BUFFER_RANGE(name) void name(GLenum Target, GLuint Index, GLuint Buffer, GLintptr Offset, GLsizeiptr Size)
typedef GL_BIND_BUFFER_RANGE(gl_bind_buffer_range);

#define GL_BIND_BUFFER_BASE(name) void name(GLenum Target, GLuint Index, GLuint Buffer)
typedef GL_BIND_BUFFER_BASE(gl_bind_buffer_base);

#define GL_BUFFER_SUB_DATA(name) void name(GLenum Target, GLintptr Offset, GLsizeiptr Size, const GLvoid *Data)
typedef GL_BUFFER_SUB_DATA(gl_buffer_sub_data);

#define GL_VERTEX_ATTRIB_POINTER(name) void name(u32 Index, s32 Size, GLenum Type,\
                                                 GLboolean Normalized, GLsizei Stride, const GLvoid *Pointer)
typedef GL_VERTEX_ATTRIB_POINTER(gl_vertex_attrib_pointer);

#define GL_VERTEX_ATTRIBI_POINTER(name) void name(u32 Index, s32 Size, GLenum Type,\
                                                   GLsizei Stride, const GLvoid *Pointer)
typedef GL_VERTEX_ATTRIBI_POINTER(gl_vertex_attribi_pointer);

#define GL_ENABLE_VERTEX_ATTRIB_ARRAY(name) void name(u32 Index)
typedef GL_ENABLE_VERTEX_ATTRIB_ARRAY(gl_enable_vertex_attrib_array);

#define GL_VERTEX_ATTRIB_DIVISOR(name) void name(u32 Index, u32 Divisor)
typedef GL_VERTEX_ATTRIB_DIVISOR(gl_vertex_attrib_divisor);

#define GL_BLEND_FUNC(name) void name(GLenum Sfactor, GLenum Dfactor)
typedef GL_BLEND_FUNC(gl_blend_func);

#define GL_CLEAR_FUNC(name) void name(GLbitfield Mask)
typedef GL_CLEAR_FUNC(gl_clear);

#define GL_CLEAR_COLOR(name) void name(GLclampf Red, GLclampf Green, GLclampf Blue, GLclampf Alpha)
typedef GL_CLEAR_COLOR(gl_clear_color);

#define GL_DRAW_ARRAYS_INSTANCED(name) void name(GLenum Mode, s32 First, GLsizei Count, GLsizei Primcount)
typedef GL_DRAW_ARRAYS_INSTANCED(gl_draw_arrays_instanced);

#define GL_GET_ACTIVE_UNIFORM(name) void name(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
typedef GL_GET_ACTIVE_UNIFORM(gl_get_active_uniform);

#define GL_POLYGON_MODE_FUNC(name) void name(GLenum Face, GLenum Mode)
typedef GL_POLYGON_MODE_FUNC(gl_polygon_mode);

#define GL_DRAW_ARRAYS(name) void name(GLenum Mode, GLint First, GLsizei Count)
typedef GL_DRAW_ARRAYS(gl_draw_arrays);

#define GL_GET_UNIFORM_BLOCK_INDEX(name) u32 name(u32 program, const GLchar *uniformBlockName)
typedef GL_GET_UNIFORM_BLOCK_INDEX(gl_get_uniform_block_index);

#define GL_UNIFORM_BLOCK_BINDING_FUNC(name) void name(u32 program, u32 uniformBlockIndex, u32 uniformBlockBinding)
typedef GL_UNIFORM_BLOCK_BINDING_FUNC(gl_uniform_block_binding);

#pragma endregion

struct renderer_api {
    gl_draw_arrays *glDrawArrays;
    gl_polygon_mode *glPolygonMode;
    gl_create_shader *glCreateShader;
    gl_shader_source *glShaderSource;
    gl_compile_shader *glCompileShader;
    gl_get_shader_iv *glGetShaderiv;
    gl_get_shader_info_log *glGetShaderInfoLog;
    gl_delete_shader *glDeleteShader;
    gl_get_uniform_location *glGetUniformLocation;
    gl_uniform_1i *glUniform1i;
    gl_uniform_1f *glUniform1f;
    gl_uniform_2f *glUniform2f;
    gl_uniform_matrix_4fv *glUniformMatrix4fv;
    gl_gen_textures *glGenTextures;
    gl_bind_texture *glBindTexture;
    gl_tex_parameter_i *glTexParameteri;
    gl_tex_image_2d *glTexImage2D;
    gl_create_program *glCreateProgram;
    gl_attach_shader *glAttachShader;
    gl_link_program *glLinkProgram;
    gl_get_program_iv *glGetProgramiv;
    gl_get_program_info_log *glGetProgramInfoLog;
    gl_use_program *glUseProgram;
    gl_get_vertex_arrays *glGenVertexArrays;
    gl_bind_vertex_array *glBindVertexArray;
    gl_gen_buffers *glGenBuffers;
    gl_bind_buffer *glBindBuffer;
    gl_buffer_data *glBufferData;
    gl_buffer_sub_data *glBufferSubData;
    gl_bind_buffer_range *glBindBufferRange;
    gl_bind_buffer_base *glBindBufferBase;
    gl_vertex_attrib_pointer *glVertexAttribPointer;
    gl_vertex_attribi_pointer *glVertexAttribIPointer;
    gl_enable_vertex_attrib_array *glEnableVertexAttribArray;
    gl_vertex_attrib_divisor *glVertexAttribDivisor;
    gl_blend_func *glBlendFunc;
    gl_clear *glClear;
    gl_clear_color *glClearColor;
    gl_draw_arrays_instanced *glDrawArraysInstanced;
    gl_get_active_uniform *glGetActiveUniform;
    gl_get_uniform_block_index *glGetUniformBlockIndex;
    gl_uniform_block_binding *glUniformBlockBinding;
};

struct game_memory {
    u64 PermanentStorageSize;
    void *PermanentStorage;

    u64 TransientStorageSize;
    void *TransientStorage;

    platform_api Platform;
    renderer_api Renderer;
};

struct key_state {
    b32 isPressed;
    b32 isProcessed;
};

struct game_input {
    key_state Up;
    key_state Down;
    key_state Left;
    key_state Right;
};

struct game_params {
    u32 ScreenWidth;
    u32 ScreenHeight;

    f32 Delta;

    game_input Input;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_params *Params)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

inline b32
StringEquals(const char *Str1, const char *Str2) {
    return strcmp(Str1, Str2) == 0;
}

inline char*
GetLastAfterDelimiter(char *String, const char Delimiter) {
    char* Result = String;

    for (char *Scan = String; *Scan; ++Scan) {
        if (*Scan == Delimiter) {
            Result = Scan + 1;
        }
    }

    return Result;
}

inline s32 
StringLength(const char *String) {
    s32 Length = 0;

    while (*String++) {
        ++Length;
    }

    return Length;
}

inline void 
ConcatenateStrings(const char *SourceA, const char *SourceB, char *Dest) {
    s32 SourceALength = StringLength(SourceA);
    for (s32 Index = 0; Index < SourceALength; ++Index) {
        *Dest++ = *SourceA++;
    }

    s32 SourceBLength = StringLength(SourceB);
    for (s32 Index = 0; Index < SourceBLength; ++Index) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}