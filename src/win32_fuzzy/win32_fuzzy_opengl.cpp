#include "..\..\generated\glad\src\glad.c"
#include "win32_fuzzy_opengl.h"
#include "fuzzy_platform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma region Shaders

inline b32
UniformKeyComparator(shader_uniform *Uniform, char *Key)
{
    b32 Result = StringEquals(Uniform->Name, Key);
    return Result;
}

inline b32
UniformKeyExists(shader_uniform *Uniform)
{
    b32 Result = (b32)Uniform->Name;
    return Result;
}

inline void
UniformKeySetter(shader_uniform *Uniform, char *Key)
{
    Uniform->Name = Key;
}

shader_uniform *
GetUniform(shader_program *ShaderProgram, char *Name)
{
    shader_uniform *Result = Get<shader_uniform, char *>(&ShaderProgram->Uniforms, Name, UniformKeyComparator);
    return Result;
}

shader_uniform *
CreateUniform(shader_program *ShaderProgram, char *Name, memory_arena *Arena)
{
    shader_uniform *Result = Create<shader_uniform, char *>(
        &ShaderProgram->Uniforms, Name, UniformKeyExists, UniformKeySetter, Arena);
    return Result;
}

u32
CreateShader(game_state *GameState, GLenum Type, char *Source)
{
    u32 Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &Source, NULL);
    glCompileShader(Shader);

    s32 IsShaderCompiled;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &IsShaderCompiled);
    if (!IsShaderCompiled)
    {
        temporary_memory ErrorLogMemory = BeginTemporaryMemory(&GameState->WorldArena);

        s32 LogLength;
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &LogLength);

        char *ErrorLog = PushString(&GameState->WorldArena, LogLength);

        glGetShaderInfoLog(Shader, LogLength, NULL, ErrorLog);

        char Output[1024];
        FormatString(Output, sizeof(Output), "%s%s%s", "Shader compilation failed:\n", ErrorLog, "\n");
        PrintOutput(Output);

        glDeleteShader(Shader);

        EndTemporaryMemory(ErrorLogMemory);
    }
    assert(IsShaderCompiled);

    return Shader;
}

u32
CreateProgram(game_state *GameState, u32 VertexShader, u32 FragmentShader)
{
    u32 Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    s32 IsProgramLinked;
    glGetProgramiv(Program, GL_LINK_STATUS, &IsProgramLinked);

    if (!IsProgramLinked)
    {
        temporary_memory ErrorLogMemory = BeginTemporaryMemory(&GameState->WorldArena);

        s32 LogLength;
        glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &LogLength);

        char *ErrorLog = PushString(&GameState->WorldArena, LogLength);

        glGetProgramInfoLog(Program, LogLength, nullptr, ErrorLog);

        char Output[1024];
        FormatString(Output, sizeof(Output), "%s%s%s", "Shader program linkage failed:\n", ErrorLog, "\n");
        PrintOutput(Output);

        EndTemporaryMemory(ErrorLogMemory);
    }
    assert(IsProgramLinked);

    return Program;
}

inline s32
GetUniformLocation(u32 ShaderProgram, const char *Name)
{
    s32 UniformLocation = glGetUniformLocation(ShaderProgram, Name);
    //assert(uniformLocation != -1);
    return UniformLocation;
}

inline void
SetShaderUniform(s32 Location, s32 Value)
{
    glUniform1i(Location, Value);
}

inline void
SetShaderUniform(s32 Location, f32 Value)
{
    glUniform1f(Location, Value);
}

inline void
SetShaderUniform(s32 Location, vec2 Value)
{
    glUniform2f(Location, Value.x, Value.y);
}

inline void
SetShaderUniform(s32 Location, vec3 Value)
{
    glUniform3f(Location, Value.x, Value.y, Value.z);
}

inline void
SetShaderUniform(s32 Location, vec4 Value)
{
    glUniform4f(Location, Value.x, Value.y, Value.z, Value.w);
}

inline void
SetShaderUniform(s32 Location, const mat4& Value)
{
    glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(Value));
}

shader_program
CreateShaderProgram(game_state *GameState, char *VertexShaderFileName, char *FragmentShaderFileName)
{
    shader_program Result = {};

    char *VertexShaderSource = (char *)ReadFile(VertexShaderFileName).Contents;
    char *FragmentShaderSource = (char *)ReadFile(FragmentShaderFileName).Contents;
    u32 VertexShader = CreateShader(Memory, GameState, GL_VERTEX_SHADER, &VertexShaderSource[0]);
    u32 FragmentShader = CreateShader(Memory, GameState, GL_FRAGMENT_SHADER, &FragmentShaderSource[0]);

    Result.ProgramHandle = CreateProgram(GameState, VertexShader, FragmentShader);

    s32 UniformCount;
    glGetProgramiv(Result.ProgramHandle, GL_ACTIVE_UNIFORMS, &UniformCount);

    Result.Uniforms.Count = (u32)UniformCount;
    Result.Uniforms.Values = PushArray<shader_uniform>(&GameState->WorldArena, Result.Uniforms.Count);

    s32 MaxUniformLength;
    glGetProgramiv(Result.ProgramHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &MaxUniformLength);

    for (s32 UniformIndex = 0; UniformIndex < UniformCount; ++UniformIndex)
    {
        GLsizei Length;
        s32 Size;
        GLenum Type;
        char *Name = PushString(&GameState->WorldArena, MaxUniformLength);
        glGetActiveUniform(Result.ProgramHandle, UniformIndex, MaxUniformLength, &Length, &Size, &Type, Name);

        shader_uniform *Uniform = CreateUniform(&Result, Name, &GameState->WorldArena);
        Uniform->Location = GetUniformLocation(Memory, Result.ProgramHandle, Uniform->Name);
    }

    return Result;
}

#pragma endregion

#pragma region Buffers

void
SetupVertexBuffer(vertex_buffer *Buffer)
{
    glGenVertexArrays(1, &Buffer->VAO);
    glBindVertexArray(Buffer->VAO);

    glGenBuffers(1, &Buffer->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer->VBO);
    glBufferData(GL_ARRAY_BUFFER, Buffer->Size, NULL, Buffer->Usage);

    for (u32 VertexSubBufferIndex = 0; VertexSubBufferIndex < Buffer->DataLayout->SubBufferCount; ++VertexSubBufferIndex)
    {
        vertex_sub_buffer *VertexSubBuffer = Buffer->DataLayout->SubBuffers + VertexSubBufferIndex;

        glBufferSubData(GL_ARRAY_BUFFER, VertexSubBuffer->Offset, VertexSubBuffer->Size, VertexSubBuffer->Data);
    }

    for (u32 VertexAttributeIndex = 0; VertexAttributeIndex < Buffer->AttributesLayout->AttributeCount; ++VertexAttributeIndex)
    {
        vertex_buffer_attribute *VertexAttribute = Buffer->AttributesLayout->Attributes + VertexAttributeIndex;

        glEnableVertexAttribArray(VertexAttribute->Index);
        if (VertexAttribute->Type == GL_UNSIGNED_INT)
        {
            glVertexAttribIPointer(VertexAttribute->Index, VertexAttribute->Size,
                VertexAttribute->Type, VertexAttribute->Stride, VertexAttribute->OffsetPointer);
        }
        else
        {
            glVertexAttribPointer(VertexAttribute->Index, VertexAttribute->Size,
                VertexAttribute->Type, VertexAttribute->Normalized, VertexAttribute->Stride, VertexAttribute->OffsetPointer);
        }
        glVertexAttribDivisor(VertexAttribute->Index, VertexAttribute->Divisor);
    }
}

#pragma endregion

DRAW_RECTANGLE(DrawRectangle)
{
    glUseProgram(GameState->BorderShaderProgram.ProgramHandle);
    glBindVertexArray(GameState->BorderVertexBuffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, GameState->BorderVertexBuffer.VBO);

    shader_uniform *ModelUniform = GetUniform(&GameState->BorderShaderProgram, "u_Model");
    shader_uniform *ColorUniform = GetUniform(&GameState->BorderShaderProgram, "u_Color");
    shader_uniform *BorderWidthUniform = GetUniform(&GameState->BorderShaderProgram, "u_BorderWidth");
    shader_uniform *WidthOverHeightUniform = GetUniform(&GameState->BorderShaderProgram, "u_WidthOverHeight");

    vec2 BorderSize = vec2(1.f, 1.f);
    SetShaderUniform(Memory, WidthOverHeightUniform->Location, BorderSize.x / BorderSize.y);

    mat4 Model = mat4(1.f);

    // todo: consolidate about game world coordinates ([0,0] is at the center)
    Model = glm::translate(Model, vec3(GameState->ScreenWidthInMeters / 2.f, GameState->ScreenHeightInMeters / 2.f, 0.f));
    Model = glm::translate(Model, vec3(6.f, 1.f, 0.f));

    Model = glm::scale(Model, vec3(BorderSize, 0.f));

    Model = glm::translate(Model, vec3(BorderSize / 2.f, 0.f));
    Model = glm::rotate(Model, glm::radians((f32)GameState->Time * 40.f), vec3(0.f, 0.f, 1.f));
    Model = glm::translate(Model, vec3(-BorderSize / 2.f, 0.f));

    SetShaderUniform(Memory, ModelUniform->Location, Model);

    vec4 Color = vec4(1.f, 1.f, 0.f, 1.f);
    SetShaderUniform(Memory, ColorUniform->Location, Color);

    // meters to (0-1) uv-range
    f32 BorderWidth = 0.1f / BorderSize.x;
    SetShaderUniform(Memory, BorderWidthUniform->Location, BorderWidth);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
