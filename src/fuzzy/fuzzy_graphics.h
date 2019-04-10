#pragma once

// todo: move this to utils
// from https://stackoverflow.com/questions/7666509/
u32 Hash(char *Value)
{
    u32 Hash = 5381;
    s32 C;

    while (C = *Value++) {
        Hash = ((Hash << 5) + Hash) + C;
    }

    return Hash;
}

struct vertex_buffer_attribute
{
    u32 Index;
    s32 Size;
    s32 Type;
    b32 Normalized;
    u32 Stride;
    u32 Divisor;
    void *OffsetPointer;
};

struct vertex_buffer_attributes_layout
{
    u32 AttributeCount;
    vertex_buffer_attribute *Attributes;
};

struct vertex_sub_buffer
{
    u32 Offset;
    u32 Size;
    void *Data;
};

struct vertex_buffer_data_layout
{
    u32 SubBufferCount;
    vertex_sub_buffer *SubBuffers;
};

struct vertex_buffer
{
    u32 VAO;
    u32 VBO;
    u32 Size;
    u32 Usage;

    vertex_buffer_data_layout *DataLayout;
    vertex_buffer_attributes_layout *AttributesLayout;
};

struct shader_uniform
{
    char *Name;
    s32 Location;

    shader_uniform *Next;
};

struct shader_program
{
    u32 ProgramHandle;

    u32 UniformCount;
    shader_uniform *Uniforms;
};

// todo: unify this with tiles
inline shader_uniform *
GetUniform(shader_program *ShaderProgram, char *Name) {
    shader_uniform *Result = nullptr;

    u32 HashValue = Hash(Name) % ShaderProgram->UniformCount;
    assert(HashValue < ShaderProgram->UniformCount);

    shader_uniform *Uniform = ShaderProgram->Uniforms + HashValue;

    do {
        if (StringEquals(Uniform->Name, Name)) {
            Result = Uniform;
            break;
        }

        Uniform = Uniform->Next;
    } while (Uniform);

    return Result;
}

inline shader_uniform *
CreateUniform(shader_program *ShaderProgram, char *Name, memory_arena *Arena)
{
    shader_uniform *Result = nullptr;

    u32 HashValue = Hash(Name) % ShaderProgram->UniformCount;
    assert(HashValue < ShaderProgram->UniformCount);

    shader_uniform *Uniform = ShaderProgram->Uniforms + HashValue;

    do {
        if (!Uniform->Name)
        {
            Result = Uniform;
            Result->Name = Name;
            break;
        }

        if (Uniform->Name && !Uniform->Next)
        {
            // todo: potentially dangerous operation
            Uniform->Next = PushStruct<shader_uniform>(Arena);
            Uniform->Next->Name = nullptr;
        }

        Uniform = Uniform->Next;
    } while (Uniform);

    return Result;
}