#pragma once

#include "fuzzy_types.h"

// from https://stackoverflow.com/questions/664014
inline u32
Hash(u32 Value)
{
    u32 Hash = Value;

    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = (Hash >> 16) ^ Hash;

    return Hash;
}

// from https://stackoverflow.com/questions/7666509/
inline u32
Hash(char *Value)
{
    u32 Hash = 5381;
    i32 C;

    while (C = *Value++)
    {
        Hash = ((Hash << 5) + Hash) + C;
    }

    return Hash;
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
    f32 Result = (1.f - t) * A + t * B;

    return Result;
}

inline f32
Square(f32 Value)
{
    f32 Result = Value * Value;

    return Result;
}

inline f32
AbsoluteValue(f32 Value)
{
    f32 Result = Value;
    if (Value < 0.f) 
    {
        Result = -Value;
    }

    return Result;
}
