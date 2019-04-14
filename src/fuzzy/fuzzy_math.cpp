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
    s32 C;

    while (C = *Value++)
    {
        Hash = ((Hash << 5) + Hash) + C;
    }

    return Hash;
}