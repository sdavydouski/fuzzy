#pragma once

template<typename T>
struct hash_table
{
    u32 Count;
    T *Values;
};

template<typename T>
struct stack
{
    u32 MaxCount;
    u32 Head;

    T *Values;
};

// circular buffer implementation
template<typename T>
struct queue
{
    u32 MaxCount;
    u32 Head;
    u32 Tail;

    T *Values;
};
