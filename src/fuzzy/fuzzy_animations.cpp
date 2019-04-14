#include "fuzzy_animations.h"
#include "fuzzy_platform.h"
#include "fuzzy.h"

inline b32
AnimationKeyComparator(animation *Animation, char *Key)
{
    b32 Result = StringEquals(Animation->Name, Key);
    return Result;
}

inline b32
AnimationKeyExists(animation *Animation)
{
    b32 Result = (b32)Animation->Name;
    return Result;
}

inline void
AnimationKeySetter(animation *Animation, char *Key)
{
    Animation->Name = Key;
}

animation *
GetAnimation(game_state *GameState, char *Name)
{
    animation *Result = Get<animation, char *>(&GameState->Animations, Name, AnimationKeyComparator);
    return Result;
}

animation *
CreateAnimation(game_state *GameState, char *Name, memory_arena *Arena)
{
    animation *Result = Create<animation, char *>(
        &GameState->Animations, Name, AnimationKeyExists, AnimationKeySetter, Arena);
    return Result;
}

inline animation_frame *
GetCurrentAnimationFrame(animation *Animation)
{
    animation_frame *Result = Animation->AnimationFrames + Animation->CurrentFrameIndex;
    return Result;
}

void
ChangeAnimation(game_state *GameState, entity *Entity, animation *Animation, b32 Loop = true)
{
    Animation->CurrentFrameIndex = 0;
    animation_frame *CurrentFrame = GetCurrentAnimationFrame(Animation);

    CurrentFrame = Animation->AnimationFrames + Animation->CurrentFrameIndex;
    CurrentFrame->CurrentXOffset01 = CurrentFrame->XOffset01;
    CurrentFrame->CurrentYOffset01 = CurrentFrame->YOffset01;
    Animation->CurrentTime = 0.f;

    if (Loop)
    {
        Animation->NextToPlay = Animation;
    }

    Entity->CurrentAnimation = Animation;
}

void
ChangeAnimation(game_state *GameState, entity *Entity, char *Name, b32 Loop = true)
{
    animation *Animation = GetAnimation(GameState, Name);
    ChangeAnimation(GameState, Entity, Animation, Loop);
}
