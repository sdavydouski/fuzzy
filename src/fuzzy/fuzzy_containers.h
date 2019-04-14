#pragma once

#include "fuzzy_types.h"
#include "fuzzy_memory.h"

#pragma region HashTable

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

#pragma endregion