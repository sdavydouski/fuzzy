#pragma once

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

    hash_table<shader_uniform> Uniforms;
};

struct rotation_info
{
    f32 AngleInRadians;
    vec3 Axis;
};