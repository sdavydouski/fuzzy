#include "fuzzy_containers.h"
#include "fuzzy_memory.h"

template<typename TValue, typename TKey>
TValue *
Get(hash_table<TValue> *HashTable, TKey Key, b32(*KeyComparator)(TValue *, TKey)) {
    TValue *Result = nullptr;

    u32 HashValueIndex = Hash(Key) % HashTable->Count;
    assert(HashValueIndex < HashTable->Count);

    TValue *Value = HashTable->Values + HashValueIndex;

    do 
    {
        if (KeyComparator(Value, Key)) 
        {
            Result = Value;
            break;
        }

        Value = Value->Next;
    } 
    while (Value);

    return Result;
}


template<typename TValue, typename TKey>
TValue * 
Create(
    hash_table<TValue> *HashTable, 
    TKey Key, 
    b32(*KeyExists)(TValue *), 
    void(*KeySetter)(TValue *, TKey), 
    memory_arena *Arena
)
{
    TValue *Result = nullptr;

    u32 HashValueIndex = Hash(Key) % HashTable->Count;
    assert(HashValueIndex < HashTable->Count);

    TValue *Value = HashTable->Values + HashValueIndex;

    do 
    {
        if (!KeyExists(Value))
        {
            Result = Value;
            KeySetter(Result, Key);
            break;
        }

        if (KeyExists(Value) && !Value->Next)
        {
            Value->Next = PushStruct<TValue>(Arena);
            KeySetter(Value->Next, 0);
        }

        Value = Value->Next;
    } 
    while (Value);

    return Result;
}