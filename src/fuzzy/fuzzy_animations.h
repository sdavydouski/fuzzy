#pragma once

#include "fuzzy_types.h"

struct animation_frame
{
    f32 XOffset01;
    f32 YOffset01;

    f32 Width01;
    f32 Height01;

    f32 CurrentXOffset01;
    f32 CurrentYOffset01;

    f32 Duration;
};

struct animation
{
    u32 AnimationFrameCount;
    animation_frame *AnimationFrames;

    u32 CurrentFrameIndex;
    f32 CurrentTime;
    b32 StopOnTheLastFrame;

    char *Name;
    
    animation *NextToPlay;
    // todo: hash_table naming
    animation *Next;
};