#pragma once

#include "fuzzy_types.h"
#include "fuzzy_memory.h"

template<typename T>
struct hash_table
{
    u32 Count;
    T *Values;
};

template<typename TValue, typename TKey>
TValue *
Get(hash_table<TValue> *HashTable, TKey Key, b32(*KeyComparator)(TValue *, TKey));

template<typename TValue, typename TKey>
TValue * 
Create(
    hash_table<TValue> *HashTable, 
    TKey Key, 
    b32(*KeyExists)(TValue *), 
    void(*KeySetter)(TValue *, TKey), 
    memory_arena *Arena
);

template<typename TValue>
struct stack
{
    u32 MaxCount;
    TValue *Values;

    u32 Count;
};

template<typename TValue>
inline void
Push(stack<TValue> *Stack, TValue NewValue);

template<typename TValue>
inline TValue *
Top(stack<TValue> *Stack);

template<typename TValue>
inline TValue *
Pop(stack<TValue> *Stack);

template<typename TValue>
inline void
Replace(stack<TValue> *Stack, TValue NewValue);