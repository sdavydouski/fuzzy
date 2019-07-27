#include "fuzzy_containers.h"
#include "fuzzy_memory.h"

template<typename TValue, typename TKey>
internal TValue *
Get(hash_table<TValue> *HashTable, TKey Key, b32(*KeyComparator)(TValue *, TKey)) {
    TValue *Result = nullptr;

    u32 HashValueIndex = Hash(Key) % HashTable->Count;
    Assert(HashValueIndex < HashTable->Count);

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
internal TValue * 
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
    Assert(HashValueIndex < HashTable->Count);

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

template<typename T>
inline void
Push(stack<T> *Stack, T NewValue)
{
    T *Value = Stack->Values + Stack->Head;
    ++Stack->Head;

    Assert(Stack->Head < Stack->MaxCount);

    *Value = NewValue;
}

template<typename T>
inline T
Pop(stack<T> *Stack)
{
    T Value = Top(Stack);
    --Stack->Head;

    Assert(Stack->Head >= 0);

    return Value;
}

template<typename T>
inline T
Top(stack<T> *Stack)
{
    Assert(Stack->Head > 0);

    T *Value = Stack->Values + (Stack->Head - 1);
    return *Value;
}

template<typename T>
inline void
Enqueue(queue<T> *Queue, T NewValue)
{
    u32 NextHead = Queue->Head + 1;

    if (NextHead >= Queue->MaxCount)
    {
        NextHead = 0;
    }

    if (NextHead == Queue->Tail)
    {
        Assert("Queue is full");
    }

    T *Value = Queue->Values + Queue->Head;
    *Value = NewValue;

    Queue->Head = NextHead;
}

template<typename T>
inline T
Dequeue(queue<T> *Queue)
{
    if (Queue->Head == Queue->Tail)
    {
        Assert("Queue is empty");
    }

    u32 NextTail = Queue->Tail + 1;

    if (NextTail >= Queue->MaxCount)
    {
        NextTail = 0;
    }

    T *Value = Queue->Values + Queue->Tail;
    Queue->Tail = NextTail;

    return *Value;
}
