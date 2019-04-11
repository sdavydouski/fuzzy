#pragma once

#include "fuzzy_types.h"

enum animation_type
{
    ANIMATION_PLAYER_IDLE,
    ANIMATION_PLAYER_RUN,
    ANIMATION_PLAYER_JUMP_UP,
    ANIMATION_PLAYER_JUMP_DOWN,
    ANIMATION_PLAYER_SQUASH,

    ANIMATION_COUNT
};

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

    animation_type Type;

    animation *Next;
};