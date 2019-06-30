#pragma once

template<typename T>
struct hash_table
{
    u32 Count;
    T *Values;
};

template<typename TValue>
struct stack
{
    u32 MaxCount;
    TValue *Values;

    u32 Count;
};
