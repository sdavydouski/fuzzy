#pragma once

#include "fuzzy_types.h"

constexpr inline u64 Kilobytes(u64 bytes) {
    u64 Result = bytes * 1024;
    return Result;
}

constexpr inline u64 Megabytes(u64 bytes) {
    u64 Result = Kilobytes(bytes) * 1024;
    return Result;
}

constexpr inline u64 Gigabytes(u64 bytes) {
    u64 Result = Megabytes(bytes) * 1024;
    return Result;
}

constexpr inline u64 Terabytes(u64 bytes) {
    u64 Result = Gigabytes(bytes) * 1024;
    return Result;
}

struct memory_arena {
    memory_index Size;

    memory_index Used;
    void* Base;
};

inline void InitializeMemoryArena(memory_arena* Arena, memory_index Size, void* Base) {
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

inline void* PushSize(memory_arena* Arena, memory_index Size) {
    assert((Arena->Used + Size) <= Arena->Size);

    void* Result = (u8*)Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

template<typename T>
inline T* PushStruct(memory_arena* Arena) {
    T* Result = (T*)PushSize(Arena, sizeof(T));
    return Result;
}

template<typename T>
inline T* PushArray(memory_arena* Arena, u32 Count) {
    T* Result = (T*)PushSize(Arena, Count * sizeof(T));
    return Result;
}