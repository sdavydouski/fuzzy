#include "fuzzy_animations.h"
#include "fuzzy_platform.h"
#include "fuzzy.h"

animation_type
GetAnimationTypeFromString(const char *String)
{
    animation_type Result;

    if (StringEquals(String, "PLAYER_IDLE"))
    {
        Result = ANIMATION_PLAYER_IDLE;
    }
    else if (StringEquals(String, "PLAYER_RUN"))
    {
        Result = ANIMATION_PLAYER_RUN;
    }
    else if (StringEquals(String, "PLAYER_JUMP_UP"))
    {
        Result = ANIMATION_PLAYER_JUMP_UP;
    }
    else if (StringEquals(String, "PLAYER_JUMP_DOWN"))
    {
        Result = ANIMATION_PLAYER_JUMP_DOWN;
    }
    else if (StringEquals(String, "PLAYER_SQUASH"))
    {
        Result = ANIMATION_PLAYER_SQUASH;
    }
    else
    {
        InvalidCodePath;
    }

    return Result;
}

inline animation_frame *
GetCurrentAnimationFrame(animation *Animation)
{
    animation_frame *Result = Animation->AnimationFrames + Animation->CurrentFrameIndex;
    return Result;
}

inline animation *
GetAnimation(game_state *GameState, animation_type Type)
{
    animation *Result = GameState->Animations + Type;
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
        Animation->Next = Animation;
    }

    Entity->CurrentAnimation = Animation;
}

void
ChangeAnimation(game_state *GameState, entity *Entity, animation_type Type, b32 Loop = true)
{
    animation *Animation = GetAnimation(GameState, Type);
    ChangeAnimation(GameState, Entity, Animation, Loop);
}