#define _CRT_SECURE_NO_WARNINGS

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// todo: for defines and such - won't be needed in future
#include "glad/glad.h"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>

#include "fuzzy_types.h"
#include "fuzzy_platform.h"

#include "fuzzy_math.cpp"
#include "fuzzy_random.cpp"
#include "fuzzy_containers.cpp"
#include "fuzzy_tiled.cpp"
#include "fuzzy_text.cpp"
#include "fuzzy_renderer.cpp"
#include "fuzzy_animations.cpp"
#include "fuzzy_assets.cpp"

#include "fuzzy.h"

global const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
global const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
global const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

inline vec3
NormalizeRGB(u32 Red, u32 Green, u32 Blue)
{
    const f32 MAX = 255.f;
    return vec3(Red / MAX, Green / MAX, Blue / MAX);
}

inline void
GetEntityStateString(entity_state State, char *String, u32 Length)
{
    char *StateString = 0;

    switch (State)
    {
    case ENTITY_STATE_IDLE:
        CopyString("idle", String, Length);
        break;
    case ENTITY_STATE_RUN:
        CopyString("run", String, Length);
        break;
    case ENTITY_STATE_JUMP:
        CopyString("jump", String, Length);
        break;
    case ENTITY_STATE_DIVE:
        CopyString("dive", String, Length);
        break;
    case ENTITY_STATE_FALL:
        CopyString("fall", String, Length);
        break;
    case ENTITY_STATE_SQUASH:
        CopyString("squash", String, Length);
        break;
    case ENTITY_STATE_DUCK:
        CopyString("duck", String, Length);
        break;
    case ENTITY_STATE_ATTACK:
        CopyString("attack", String, Length);
        break;
    default:
        break;
    }
}

// todo: write more efficient functions
inline f32
Clamp(f32 Value, f32 Min, f32 Max)
{
    if (Value < Min) return Min;
    if (Value > Max) return Max;

    return Value;
}

inline f32
GetRandomInRange(f32 Min, f32 Max)
{
    f32 Result = Min + (f32)(rand()) / ((f32)(RAND_MAX / (Max - Min)));
    return Result;
}

inline b32
IntersectAABB(const aabb& Box1, const aabb& Box2)
{
    // Separating Axis Theorem
    b32 XCollision = Box1.Position.x + Box1.Size.x > Box2.Position.x && Box1.Position.x < Box2.Position.x + Box2.Size.x;
    b32 YCollision = Box1.Position.y + Box1.Size.y > Box2.Position.y && Box1.Position.y < Box2.Position.y + Box2.Size.y;

    return XCollision && YCollision;
}

// basic Minkowski-based collision detection
internal vec2
SweptAABB(const vec2 Point, const vec2 Delta, const aabb& Box, const vec2 Padding)
{
    vec2 Time = vec2(1.f);

    f32 LeftTime = 1.f;
    f32 RightTime = 1.f;
    f32 TopTime = 1.f;
    f32 BottomTime = 1.f;

    vec2 Position = Box.Position - Padding;
    vec2 Size = Box.Size + Padding;

    if (Delta.x != 0.f && Position.y < Point.y && Point.y < Position.y + Size.y)
    {
        LeftTime = (Position.x - Point.x) / Delta.x;
        if (LeftTime < Time.x)
        {
            Time.x = LeftTime;
        }

        RightTime = (Position.x + Size.x - Point.x) / Delta.x;
        if (RightTime < Time.x)
        {
            Time.x = RightTime;
        }
    }

    if (Delta.y != 0.f && Position.x < Point.x && Point.x < Position.x + Size.x)
    {
        TopTime = (Position.y - Point.y) / Delta.y;
        if (TopTime < Time.y)
        {
            Time.y = TopTime;
        }

        BottomTime = (Position.y + Size.y - Point.y) / Delta.y;
        if (BottomTime < Time.y)
        {
            Time.y = BottomTime;
        }
    }

    return Time;
}

inline entity_state
GetCurrentEntityState(entity *Entity)
{
    entity_state Result = Top(&Entity->StatesStack);

    return Result;
}

inline void
EmitEvent(game_state *GameState, event_type Type)
{
    event Event = {};
    Event.Type = Type;

    Enqueue(&GameState->EventQueue, Event);
}

inline void
EmitEvent(game_state *GameState, event_type Type, event_data Data)
{
    event Event = {};
    Event.Type = Type;
    Event.Data = PushStruct<event_data>(&GameState->WorldArena);

    Enqueue(&GameState->EventQueue, Event);
}

internal void
ProcessInput(game_state *GameState, game_input *Input, f32 Delta)
{
    entity_state PlayerState = GetCurrentEntityState(GameState->Player);

    f32 JumpAcceleration = 20.f;
    f32 RunAcceleration = 10.f;

    // todo: move out common parts
    switch (PlayerState)
    {
    case ENTITY_STATE_IDLE:
        if (Input->Left.isPressed || Input->Right.isPressed)
        {
            Push(&GameState->Player->StatesStack, ENTITY_STATE_RUN);
        }
        if (Input->Jump.isPressed && !Input->Jump.isProcessed)
        {
            Input->Jump.isProcessed = true;

            GameState->Player->Acceleration.y = JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;
        }
        if (Input->Down.isPressed)
        {
            Push(&GameState->Player->StatesStack, ENTITY_STATE_DUCK);
        }
        if (Input->Attack.isPressed && !Input->Attack.isProcessed)
        {
            Input->Attack.isProcessed = true;

            Push(&GameState->Player->StatesStack, ENTITY_STATE_ATTACK);
        }
        break;
    case ENTITY_STATE_RUN:
        if (!Input->Left.isPressed && !Input->Right.isPressed)
        {
            Pop(&GameState->Player->StatesStack);
        }
        if (Input->Jump.isPressed && !Input->Jump.isProcessed)
        {
            Input->Jump.isProcessed = true;

            // todo: duplicate
            GameState->Player->Acceleration.y = JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;
        }
        if (Input->Down.isPressed)
        {
            Push(&GameState->Player->StatesStack, ENTITY_STATE_DUCK);
        }
        if (Input->Attack.isPressed && !Input->Attack.isProcessed)
        {
            Input->Attack.isProcessed = true;

            Push(&GameState->Player->StatesStack, ENTITY_STATE_ATTACK);
        }
        break;
    case ENTITY_STATE_JUMP:
        if (Input->Attack.isPressed && !Input->Attack.isProcessed)
        {
            Input->Attack.isProcessed = true;

            //Push(&GameState->Player->StatesStack, ENTITY_STATE_ATTACK);
        }
        if (Input->Jump.isPressed && !Input->Jump.isProcessed)
        {
            Input->Jump.isProcessed = true;

            GameState->Player->Acceleration.y = JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;
        }
        if (Input->Down.isPressed)
        {
            GameState->Player->Acceleration.y = -JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;

            Pop(&GameState->Player->StatesStack);
            Push(&GameState->Player->StatesStack, ENTITY_STATE_DIVE);
        }
        break;
    case ENTITY_STATE_FALL:
        if (Input->Attack.isPressed && !Input->Attack.isProcessed)
        {
            Input->Attack.isProcessed = true;

            //Push(&GameState->Player->StatesStack, ENTITY_STATE_ATTACK);
        }
        if (Input->Jump.isPressed && !Input->Jump.isProcessed)
        {
            Input->Jump.isProcessed = true;

            GameState->Player->Acceleration.y = JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;
        }
        if (Input->Down.isPressed)
        {
            GameState->Player->Acceleration.y = -JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;

            Pop(&GameState->Player->StatesStack);
            Push(&GameState->Player->StatesStack, ENTITY_STATE_DIVE);
        }
        break;
    case ENTITY_STATE_DUCK:
        if (!Input->Down.isPressed)
        {
            Pop(&GameState->Player->StatesStack);
        }
        break;
    case ENTITY_STATE_ATTACK:
        break;
    case ENTITY_STATE_SQUASH:
        if (Input->Jump.isPressed && !Input->Jump.isProcessed)
        {
            Input->Jump.isProcessed = true;

            GameState->Player->Acceleration.y = JumpAcceleration;
            GameState->Player->Velocity.y = 0.f;

            Pop(&GameState->Player->StatesStack);
            //Push(&GameState->Player->StatesStack, ENTITY_STATE_JUMP);
        }
        break;
    //case ENTITY_STATE_DIVE:
    //    if (Input->Down.isPressed)
    //    {
    //        GameState->Player->Acceleration.y = -JumpAcceleration;
    //        GameState->Player->Velocity.y = 0.f;

    //        Pop(&GameState->Player->StatesStack);
    //    }
    //    break;
    default:
        break;
    }

    if (Input->Left.isPressed)
    {
        if (PlayerState != ENTITY_STATE_DUCK)
        {
            GameState->Player->Acceleration.x = -RunAcceleration;

            // todo: in future handle flipped vertically/diagonally
            GameState->Player->RenderInfo->Flipped = true;
        }
    }

    if (Input->Right.isPressed)
    {
        if (PlayerState != ENTITY_STATE_DUCK)
        {
            GameState->Player->Acceleration.x = RunAcceleration;

            GameState->Player->RenderInfo->Flipped = false;
        }
    }

    f32 ZoomScale = 0.2f;

    // todo: float precision!
    if (Input->ScrollY == 0.f)
    {
        //GameState->Zoom = 1.f;
    }
    else if (Input->ScrollY > 0.f)
    {
        GameState->Zoom = 1.f / (1.f + Input->ScrollY * ZoomScale);
    }
    else if (Input->ScrollY < 0.f)
    {
        GameState->Zoom = 1.f + abs(Input->ScrollY) * ZoomScale;
    }
}

inline vec2
GetUVOffset01FromTileID(tileset *Tileset, u32 TileID)
{
    i32 TileX = TileID % Tileset->Columns;
    i32 TileY = TileID / Tileset->Columns;

    vec2 Result = vec2(
        (f32)(TileX * (Tileset->TileWidthInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Width,
        (f32)(TileY * (Tileset->TileHeightInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Height
    );

    return Result;
}

internal void
GameInit(game_state *GameState, game_memory *Memory, game_params *Params)
{
    platform_api *Platform = &Memory->Platform;
    renderer_api *Renderer = &Memory->Renderer;

    i32 ScreenWidth = Params->ScreenWidth;
    i32 ScreenHeight = Params->ScreenHeight;
    vec2 ScreenCenter = vec2(ScreenWidth / 2.f, ScreenHeight / 2.f);

    InitializeMemoryArena(
        &GameState->WorldArena,
        Memory->PermanentStorageSize - sizeof(game_state),
        (u8*)Memory->PermanentStorage + sizeof(game_state)
    );

    LoadGameAssets(&Memory->Platform, GameState, &GameState->WorldArena);

    // initialize event queue (todo: move to separate function)
    GameState->EventQueue = {};
    GameState->EventQueue.Head = 0;
    GameState->EventQueue.Tail = 0;
    GameState->EventQueue.MaxCount = 10;
    GameState->EventQueue.Values = PushArray<event>(&GameState->WorldArena, GameState->EventQueue.MaxCount);

    GameState->CurrentFont = GameState->FontAssets + 1;

    GameState->ScreenWidthInWorldUnits = 20.f;
    f32 MetersToPixels = (f32)ScreenWidth / GameState->ScreenWidthInWorldUnits;
    f32 PixelsToWorldUnits = 1.f / MetersToPixels;

    GameState->WorldUnitsToPixels = MetersToPixels;
    GameState->PixelsToWorldUnits = PixelsToWorldUnits;

    GameState->ScreenHeightInWorldUnits = ScreenHeight * PixelsToWorldUnits;

    char *MapJson = (char *)Platform->ReadFile("maps/map01.json").Contents;
    GameState->Map = {};
    LoadMap(&GameState->Map, MapJson, &GameState->WorldArena, Platform);

    tile_meta_info *TileInfo = GetTileMetaInfo(&GameState->Map.Tilesets[0].Source, 544);

    tileset *Tileset = &GameState->Map.Tilesets[0].Source;
    u32 TilesetFirstGID = GameState->Map.Tilesets[0].FirstGID;

    char *OpenGLVersion = (char*)Renderer->glGetString(GL_VERSION);
    Platform->PrintOutput(OpenGLVersion);
    Platform->PrintOutput("\n");

    // tileset texture
    Renderer->glGenTextures(1, &GameState->TilesetTexture);
    Renderer->glBindTexture(GL_TEXTURE_2D, GameState->TilesetTexture);

    // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
    // since we do not use mipmaps we must override this value
    Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Tileset->Image.Width, Tileset->Image.Height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, Tileset->Image.Memory);

    // font atlas texture
    Renderer->glGenTextures(1, &GameState->FontTextureAtlas);
    Renderer->glBindTexture(GL_TEXTURE_2D, GameState->FontTextureAtlas);

    Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, GameState->CurrentFont->TextureAtlas.Width, GameState->CurrentFont->TextureAtlas.Height,
        0, GL_RED, GL_UNSIGNED_BYTE, GameState->CurrentFont->TextureAtlas.Memory);

    vec2 TileSize01 = vec2((f32)Tileset->TileWidthInPixels / (f32)Tileset->Image.Width,
        (f32)Tileset->TileHeightInPixels / (f32)Tileset->Image.Height);

    const u32 transformsBindingPoint = 0;

    {
        GameState->TilesShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/tile_instanced.vert", "shaders/tile_instanced.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->TilesShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->TilesShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);

        Renderer->glUseProgram(GameState->TilesShaderProgram.ProgramHandle);

        shader_uniform *TileSizeUniform = GetUniform(&GameState->TilesShaderProgram, "u_TileSize");
        SetShaderUniform(Renderer, TileSizeUniform->Location, TileSize01);
    }

    {
        GameState->BoxesShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/box_instanced.vert", "shaders/box_instanced.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->BoxesShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->BoxesShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);
    }

    {
        GameState->DrawableEntitiesShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/entity_instanced.vert", "shaders/entity_instanced.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->DrawableEntitiesShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->DrawableEntitiesShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);

        Renderer->glUseProgram(GameState->DrawableEntitiesShaderProgram.ProgramHandle);

        shader_uniform *TileSizeUniform = GetUniform(&GameState->DrawableEntitiesShaderProgram, "u_TileSize");
        SetShaderUniform(Renderer, TileSizeUniform->Location, TileSize01);
    }

    {
        GameState->DrawableEntitiesBorderShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/entity_instanced.vert", "shaders/color.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->DrawableEntitiesBorderShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->DrawableEntitiesBorderShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);
    }

    {
        GameState->RectangleOutlineShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/rectangle.vert", "shaders/rectangle_outline.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->RectangleOutlineShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->RectangleOutlineShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);
    }

    {
        GameState->RectangleShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/rectangle.vert", "shaders/rectangle.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->RectangleShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->RectangleShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);
    }

    {
        GameState->ParticlesShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/particle_instanced.vert", "shaders/particle_instanced.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->ParticlesShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->ParticlesShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);

        Renderer->glUseProgram(GameState->ParticlesShaderProgram.ProgramHandle);

        /*shader_uniform *TileSizeUniform = GetUniform(&GameState->ParticlesShaderProgram, "u_TileSize");
        SetShaderUniform(Memory, TileSizeUniform->Location, TileSize01);*/
    }

    {
        GameState->SpriteShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/sprite.vert", "shaders/sprite.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->SpriteShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->SpriteShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);

        Renderer->glUseProgram(GameState->SpriteShaderProgram.ProgramHandle);

        shader_uniform *TileSizeUniform = GetUniform(&GameState->SpriteShaderProgram, "u_TileSize");
        SetShaderUniform(Renderer, TileSizeUniform->Location, TileSize01);
    }

    {
        GameState->TextShaderProgram = 
            CreateShaderProgram(Renderer, Platform, GameState, "shaders/text.vert", "shaders/text.frag");

        u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->TextShaderProgram.ProgramHandle, "transforms");
        Renderer->glUniformBlockBinding(GameState->TextShaderProgram.ProgramHandle, transformsUniformBlockIndex, transformsBindingPoint);
    }


    Renderer->glGenBuffers(1, &GameState->UBO);
    Renderer->glBindBuffer(GL_UNIFORM_BUFFER, GameState->UBO);
    Renderer->glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), NULL, GL_STREAM_DRAW);
    Renderer->glBindBufferBase(GL_UNIFORM_BUFFER, transformsBindingPoint, GameState->UBO);

    f32 QuadVerticesData[] = {
        // Pos     // UV
        0.f, 0.f,  0.f, 1.f,
        0.f, 1.f,  0.f, 0.f,
        1.f, 0.f,  1.f, 1.f,
        1.f, 1.f,  1.f, 0.f
    };

    u32 QuadVerticesSize = ArrayCount(QuadVerticesData) * sizeof(f32);
    GameState->QuadVerticesSize = QuadVerticesSize;

    f32 *QuadVertices = PushArray<f32>(&GameState->WorldArena, ArrayCount(QuadVerticesData));
    for (u32 Index = 0; Index < ArrayCount(QuadVerticesData); ++Index)
    {
        f32 *QuadVertex = QuadVertices + Index;
        *QuadVertex = QuadVerticesData[Index];
    }

    // todo: in future all these counts will be in asset pack file 
    GameState->TotalTileCount = 0;
    GameState->TotalBoxCount = 0;
    for (u32 TileLayerIndex = 0; TileLayerIndex < GameState->Map.TileLayerCount; ++TileLayerIndex)
    {
        tile_layer* TileLayer = GameState->Map.TileLayers + TileLayerIndex;

        if (TileLayer->Visible)
        {
            for (u32 ChunkIndex = 0; ChunkIndex < TileLayer->ChunkCount; ++ChunkIndex)
            {
                map_chunk *Chunk = TileLayer->Chunks + ChunkIndex;

                for (u32 GIDIndex = 0; GIDIndex < Chunk->GIDCount; ++GIDIndex)
                {
                    u32 GID = Chunk->GIDs[GIDIndex];
                    if (GID > 0)
                    {
                        ++GameState->TotalTileCount;

                        tile_meta_info* TileInfo = GetTileMetaInfo(Tileset, GID - TilesetFirstGID);
                        if (TileInfo)
                        {
                            GameState->TotalBoxCount += TileInfo->BoxCount;
                        }
                    }
                }
            }
        }
    }

    GameState->TotalObjectCount = 0;
    GameState->TotalDrawableObjectCount = 0;
    for (u32 ObjectLayerIndex = 0; ObjectLayerIndex < GameState->Map.ObjectLayerCount; ++ObjectLayerIndex)
    {
        object_layer *ObjectLayer = GameState->Map.ObjectLayers + ObjectLayerIndex;

        if (ObjectLayer->Visible)
        {
            u32 ObjectCount = ObjectLayer->ObjectCount;
            GameState->TotalObjectCount += ObjectCount;

            for (u32 ObjectIndex = 0; ObjectIndex < ObjectCount; ++ObjectIndex)
            {
                map_object* Object = ObjectLayer->Objects + ObjectIndex;

                if (Object->GID)
                {
                    ++GameState->TotalDrawableObjectCount;

                    u32 TileID = Object->GID - TilesetFirstGID;
                    tile_meta_info* TileInfo = GetTileMetaInfo(Tileset, TileID);

                    if (TileInfo)
                    {
                        GameState->TotalBoxCount += TileInfo->BoxCount;
                    }
                }
            }
        }
    }

    GameState->Animations.Count = 0;

    for (u32 TilesetIndex = 0; TilesetIndex < GameState->Map.TilesetCount; ++TilesetIndex)
    {
        tileset_source *TilesetSource = GameState->Map.Tilesets + TilesetIndex;
        tileset Tileset = TilesetSource->Source;

        for (u32 TileIndex = 0; TileIndex < Tileset.Tiles.Count; ++TileIndex)
        {
            tile_meta_info *Tile = Tileset.Tiles.Values + TileIndex;

            if (Tile->AnimationFrameCount > 0)
            {
                ++GameState->Animations.Count;
            }
        }
    }

    GameState->Animations.Values = PushArray<animation>(&GameState->WorldArena, GameState->Animations.Count);

    for (u32 TilesetIndex = 0; TilesetIndex < GameState->Map.TilesetCount; ++TilesetIndex)
    {
        tileset_source *TilesetSource = GameState->Map.Tilesets + TilesetIndex;
        tileset Tileset = TilesetSource->Source;

        for (u32 TileIndex = 0; TileIndex < Tileset.Tiles.Count; ++TileIndex)
        {
            tile_meta_info *Tile = Tileset.Tiles.Values + TileIndex;

            if (Tile->AnimationFrameCount > 0)
            {
                char *AnimationName = 0;
                b32 StopOnTheLastFrame = false;

                for (u32 CustomPropertyIndex = 0; CustomPropertyIndex < Tile->CustomPropertiesCount; ++CustomPropertyIndex)
                {
                    tile_custom_property *CustomProperty = Tile->CustomProperties + CustomPropertyIndex;

                    if (StringEquals(CustomProperty->Name, "AnimationName"))
                    {
                        AnimationName = (char *)CustomProperty->Value;
                    }

                    if (StringEquals(CustomProperty->Name, "StopOnTheLastFrame"))
                    {
                        StopOnTheLastFrame = (bool)CustomProperty->Value;
                    }
                }

                Assert(AnimationName);

                animation *Animation = CreateAnimation(GameState, AnimationName, &GameState->WorldArena);

                Animation->Name = AnimationName;
                Animation->StopOnTheLastFrame = StopOnTheLastFrame;
                Animation->CurrentFrameIndex = 0;
                Animation->AnimationFrameCount = Tile->AnimationFrameCount;
                Animation->AnimationFrames = PushArray<animation_frame>(&GameState->WorldArena, Animation->AnimationFrameCount);

                for (u32 AnimationFrameIndex = 0; AnimationFrameIndex < Tile->AnimationFrameCount; ++AnimationFrameIndex)
                {
                    tile_animation_frame *TileAnimationFrame = Tile->AnimationFrames + AnimationFrameIndex;
                    animation_frame *AnimationFrame = Animation->AnimationFrames + AnimationFrameIndex;

                    AnimationFrame->Duration = (f32)TileAnimationFrame->Duration;
                    AnimationFrame->Width01 = (f32)Tileset.TileWidthInPixels / (f32)Tileset.Image.Width;
                    AnimationFrame->Height01 = (f32)Tileset.TileHeightInPixels / (f32)Tileset.Image.Height;

                    vec2 UVOffset = GetUVOffset01FromTileID(&Tileset, TileAnimationFrame->TileId);
                    AnimationFrame->XOffset01 = UVOffset.x;
                    AnimationFrame->YOffset01 = UVOffset.y;

                    AnimationFrame->CurrentXOffset01 = AnimationFrame->XOffset01;
                    AnimationFrame->CurrentYOffset01 = AnimationFrame->YOffset01;
                }
            }
        }
    }

    vec2 ScreenCenterInWorldUnits = vec2(
        GameState->ScreenWidthInWorldUnits / 2.f,
        GameState->ScreenHeightInWorldUnits / 2.f
    );

    mat4 *TileInstanceModels = PushArray<mat4>(&GameState->WorldArena, GameState->TotalTileCount);
    vec2 *TileInstanceUVOffsets01 = PushArray<vec2>(&GameState->WorldArena, GameState->TotalTileCount);

    GameState->Boxes = PushArray<aabb>(&GameState->WorldArena, GameState->TotalBoxCount);
    mat4 *BoxInstanceModels = PushArray<mat4>(&GameState->WorldArena, GameState->TotalBoxCount);

    u32 TileInstanceIndex = 0;
    u32 BoxIndex = 0;
    for (u32 TileLayerIndex = 0; TileLayerIndex < GameState->Map.TileLayerCount; ++TileLayerIndex)
    {
        tile_layer *TileLayer = GameState->Map.TileLayers + TileLayerIndex;

        if (TileLayer->Visible)
        {
            for (u32 ChunkIndex = 0; ChunkIndex < TileLayer->ChunkCount; ++ChunkIndex)
            {
                map_chunk *Chunk = TileLayer->Chunks + ChunkIndex;

                for (u32 GIDIndex = 0; GIDIndex < Chunk->GIDCount; ++GIDIndex)
                {
                    u32 GID = Chunk->GIDs[GIDIndex];
                    if (GID > 0)
                    {
                        u32 TileID = GID - TilesetFirstGID;

                        // TileInstanceModel
                        mat4* TileInstanceModel = TileInstanceModels + TileInstanceIndex;
                        *TileInstanceModel = mat4(1.f);

                        i32 TileMapX = Chunk->X + (GIDIndex % Chunk->Width);
                        i32 TileMapY = Chunk->Y - (GIDIndex / Chunk->Height);

                        f32 TileXMeters = ScreenCenterInWorldUnits.x + TileMapX * Tileset->TileWidthInWorldUnits;
                        f32 TileYMeters = ScreenCenterInWorldUnits.y + TileMapY * Tileset->TileHeightInWorldUnits;

                        *TileInstanceModel = translate(*TileInstanceModel, vec3(TileXMeters, TileYMeters, 0.f));
                        *TileInstanceModel = scale(*TileInstanceModel,
                            vec3(Tileset->TileWidthInWorldUnits, Tileset->TileHeightInWorldUnits, 0.f));

                        // TileInstanceUVOffset01
                        vec2 * TileInstanceUVOffset01 = TileInstanceUVOffsets01 + TileInstanceIndex;
                        *TileInstanceUVOffset01 = GetUVOffset01FromTileID(Tileset, TileID);

                        tile_meta_info * TileInfo = GetTileMetaInfo(Tileset, TileID);
                        if (TileInfo)
                        {
                            // Box
                            for (u32 CurrentBoxIndex = 0; CurrentBoxIndex < TileInfo->BoxCount; ++CurrentBoxIndex)
                            {
                                aabb* Box = GameState->Boxes + BoxIndex;

                                Box->Position.x = TileXMeters +
                                    TileInfo->Boxes[CurrentBoxIndex].Position.x * Tileset->TilesetWidthPixelsToWorldUnits;
                                Box->Position.y = TileYMeters +
                                    ((Tileset->TileHeightInPixels -
                                        TileInfo->Boxes[CurrentBoxIndex].Position.y -
                                        TileInfo->Boxes[CurrentBoxIndex].Size.y) * Tileset->TilesetHeightPixelsToWorldUnits);

                                Box->Size.x = TileInfo->Boxes[CurrentBoxIndex].Size.x * Tileset->TilesetWidthPixelsToWorldUnits;
                                Box->Size.y = TileInfo->Boxes[CurrentBoxIndex].Size.y * Tileset->TilesetHeightPixelsToWorldUnits;

                                mat4 * BoxInstanceModel = BoxInstanceModels + BoxIndex;
                                *BoxInstanceModel = mat4(1.f);

                                *BoxInstanceModel = translate(*BoxInstanceModel,
                                    vec3(Box->Position.x, Box->Position.y, 0.f));
                                *BoxInstanceModel = scale(*BoxInstanceModel,
                                    vec3(Box->Size.x, Box->Size.y, 0.f));

                                ++BoxIndex;
                            }
                        }

                        ++TileInstanceIndex;
                    }
                }
            }
        }
    }

    // todo: i don't like the concept of entities and separate drawable entities
    // think about this
    entity *Entities = PushArray<entity>(&GameState->WorldArena, GameState->TotalObjectCount);

    GameState->EntityRenderInfoCount = GameState->TotalDrawableObjectCount;
    GameState->EntityRenderInfos = PushArray<entity_render_info>(&GameState->WorldArena, GameState->EntityRenderInfoCount);

    GameState->DrawableEntities = PushArray<entity>(&GameState->WorldArena, GameState->TotalDrawableObjectCount);

    u32 EntityInstanceIndex = 0;
    u32 BoxModelOffset = QuadVerticesSize;
    for (u32 ObjectLayerIndex = 0; ObjectLayerIndex < GameState->Map.ObjectLayerCount; ++ObjectLayerIndex)
    {
        object_layer *ObjectLayer = GameState->Map.ObjectLayers + ObjectLayerIndex;

        if (ObjectLayer->Visible)
        {
            for (u32 ObjectIndex = 0; ObjectIndex < ObjectLayer->ObjectCount; ++ObjectIndex)
            {
                map_object* Object = ObjectLayer->Objects + ObjectIndex;
                entity* Entity = Entities + EntityInstanceIndex;

                *Entity = {};
                Entity->ID = Object->ID;
                // tile objects have their position at bottom-left (https://github.com/bjorn/tiled/issues/91)
                Entity->Position = vec2(
                    Object->X * Tileset->TilesetWidthPixelsToWorldUnits,
                    (Object->Y + Object->Height) * Tileset->TilesetWidthPixelsToWorldUnits
                );
                Entity->Size = vec2(
                    Object->Width * Tileset->TilesetHeightPixelsToWorldUnits,
                    Object->Height * Tileset->TilesetHeightPixelsToWorldUnits
                );

                Entity->Type = Object->Type;

                entity_render_info * EntityRenderInfo = GameState->EntityRenderInfos + EntityInstanceIndex;
                // todo: very fragile (deal with QuadVerticesSize part)!
                EntityRenderInfo->Offset = EntityInstanceIndex * sizeof(entity_render_info) + QuadVerticesSize;

                BoxModelOffset += BoxIndex * sizeof(mat4);
                EntityRenderInfo->BoxModelOffset = BoxModelOffset;

                if (Object->GID)
                {
                    u32 TileID = Object->GID - TilesetFirstGID;

                    // EntityInstanceModel
                    EntityRenderInfo->InstanceModel = mat4(1.f);

                    f32 EntityWorldXInWorldUnits = ScreenCenterInWorldUnits.x + Entity->Position.x;
                    f32 EntityWorldYInWorldUnits = ScreenCenterInWorldUnits.y + Entity->Position.y;

                    EntityRenderInfo->InstanceModel = translate(EntityRenderInfo->InstanceModel,
                        vec3(EntityWorldXInWorldUnits, EntityWorldYInWorldUnits, 0.f));
                    EntityRenderInfo->InstanceModel = scale(EntityRenderInfo->InstanceModel,
                        vec3(Entity->Size.x, Entity->Size.y, 0.f));

                    // EntityInstanceUVOffset01
                    EntityRenderInfo->InstanceUVOffset01 = GetUVOffset01FromTileID(Tileset, TileID);

                    tile_meta_info * EntityTileInfo = GetTileMetaInfo(Tileset, TileID);

                    if (EntityTileInfo)
                    {
                        Entity->BoxCount = EntityTileInfo->BoxCount;
                        Entity->Boxes = PushArray<aabb_info>(&GameState->WorldArena, Entity->BoxCount);

                        // Box
                        for (u32 CurrentBoxIndex = 0; CurrentBoxIndex < EntityTileInfo->BoxCount; ++CurrentBoxIndex)
                        {
                            aabb* Box = GameState->Boxes + BoxIndex;

                            Box->Position.x = EntityWorldXInWorldUnits +
                                EntityTileInfo->Boxes[CurrentBoxIndex].Position.x * Tileset->TilesetWidthPixelsToWorldUnits;
                            Box->Position.y = EntityWorldYInWorldUnits +
                                ((Tileset->TileHeightInPixels -
                                    EntityTileInfo->Boxes[CurrentBoxIndex].Position.y -
                                    EntityTileInfo->Boxes[CurrentBoxIndex].Size.y) * Tileset->TilesetHeightPixelsToWorldUnits);

                            Box->Size.x = EntityTileInfo->Boxes[CurrentBoxIndex].Size.x * Tileset->TilesetWidthPixelsToWorldUnits;
                            Box->Size.y = EntityTileInfo->Boxes[CurrentBoxIndex].Size.y * Tileset->TilesetHeightPixelsToWorldUnits;

                            mat4 * BoxInstanceModel = BoxInstanceModels + BoxIndex;
                            *BoxInstanceModel = mat4(1.f);

                            *BoxInstanceModel = translate(*BoxInstanceModel,
                                vec3(Box->Position.x, Box->Position.y, 0.f));
                            *BoxInstanceModel = scale(*BoxInstanceModel,
                                vec3(Box->Size.x, Box->Size.y, 0.f));

                            Entity->Boxes[CurrentBoxIndex].Box = Box;
                            Entity->Boxes[CurrentBoxIndex].Model = BoxInstanceModel;

                            ++BoxIndex;
                        }
                    }

                    Entity->RenderInfo = EntityRenderInfo;

                    // DrawableEntity
                    entity* DrawableEntity = GameState->DrawableEntities + EntityInstanceIndex;
                    // todo: hmm...
                    *DrawableEntity = *Entity;

                    // todo:
                    if (DrawableEntity->Type == ENTITY_PLAYER)
                    {
                        GameState->Player = DrawableEntity;
                        GameState->Player->StatesStack.MaxCount = 10;
                        GameState->Player->StatesStack.Values = 
                            PushArray<entity_state>(&GameState->WorldArena, GameState->Player->StatesStack.MaxCount);

                        Push(&GameState->Player->StatesStack, ENTITY_STATE_IDLE);
                        ChangeAnimation(GameState, GameState->Player, "PLAYER_IDLE");
                    }

                    if (DrawableEntity->Type == ENTITY_SIREN)
                    {
                        DrawableEntity->StatesStack.MaxCount = 10;
                        DrawableEntity->StatesStack.Values = 
                            PushArray<entity_state>(&GameState->WorldArena, DrawableEntity->StatesStack.MaxCount);

                        Push(&DrawableEntity->StatesStack, ENTITY_STATE_IDLE);
                        ChangeAnimation(GameState, DrawableEntity, "SIREN");
                    }

                    ++EntityInstanceIndex;
                }
            }
        }
    }

    /*
    Chunk-based rendering.

    Divide your map into chunks.
    (Small sized squares of tiles; something like 32x32 tiles would work.)
    Make a separate VBO (possibly IBO) for each chunk.
    Only draw visible chunks.
    When a chunk is modified, update it's VBO accordingly.
    If you want an infinite map, you'll have to create and destroy chunks on the fly. Otherwise it shouldn't be necessary.
    */

#pragma region Tiles
    GameState->TilesVertexBuffer = {};
    GameState->TilesVertexBuffer.Size = QuadVerticesSize + GameState->TotalTileCount * (sizeof(mat4) + sizeof(vec2));
    GameState->TilesVertexBuffer.Usage = GL_STATIC_DRAW;

    GameState->TilesVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->TilesVertexBuffer.DataLayout->SubBufferCount = 3;
    GameState->TilesVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->TilesVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->TilesVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->TilesVertexBuffer.DataLayout->SubBuffers + 1;
        SubBuffer->Offset = QuadVerticesSize;
        SubBuffer->Size = GameState->TotalTileCount * sizeof(mat4);
        SubBuffer->Data = TileInstanceModels;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->TilesVertexBuffer.DataLayout->SubBuffers + 2;
        SubBuffer->Offset = QuadVerticesSize + GameState->TotalTileCount * sizeof(mat4);
        SubBuffer->Size = GameState->TotalTileCount * sizeof(vec2);
        SubBuffer->Data = TileInstanceUVOffsets01;
    }

    GameState->TilesVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->TilesVertexBuffer.AttributesLayout->AttributeCount = 6;
    GameState->TilesVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->TilesVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 1;
        Attribute->Index = 1;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize);
    }

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 2;
        Attribute->Index = 2;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 3;
        Attribute->Index = 3;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + 2 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 4;
        Attribute->Index = 4;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + 3 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->TilesVertexBuffer.AttributesLayout->Attributes + 5;
        Attribute->Index = 5;
        Attribute->Size = 2;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec2);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + GameState->TotalTileCount * sizeof(mat4));
    }

    SetupVertexBuffer(Renderer, &GameState->TilesVertexBuffer);
#pragma endregion

#pragma region Tile Boxes
    GameState->BoxesVertexBuffer = {};
    GameState->BoxesVertexBuffer.Size = QuadVerticesSize + GameState->TotalBoxCount * sizeof(mat4);
    GameState->BoxesVertexBuffer.Usage = GL_STREAM_DRAW;

    GameState->BoxesVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->BoxesVertexBuffer.DataLayout->SubBufferCount = 2;
    GameState->BoxesVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->BoxesVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->BoxesVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->BoxesVertexBuffer.DataLayout->SubBuffers + 1;
        SubBuffer->Offset = QuadVerticesSize;
        SubBuffer->Size = GameState->TotalBoxCount * sizeof(mat4);
        SubBuffer->Data = BoxInstanceModels;
    }

    GameState->BoxesVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->BoxesVertexBuffer.AttributesLayout->AttributeCount = 5;
    GameState->BoxesVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->BoxesVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->BoxesVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    {
        vertex_buffer_attribute *Attribute = GameState->BoxesVertexBuffer.AttributesLayout->Attributes + 1;
        Attribute->Index = 1;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize);
    }

    {
        vertex_buffer_attribute *Attribute = GameState->BoxesVertexBuffer.AttributesLayout->Attributes + 2;
        Attribute->Index = 2;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->BoxesVertexBuffer.AttributesLayout->Attributes + 3;
        Attribute->Index = 3;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + 2 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->BoxesVertexBuffer.AttributesLayout->Attributes + 4;
        Attribute->Index = 4;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(mat4);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + 3 * sizeof(vec4));
    }

    SetupVertexBuffer(Renderer, &GameState->BoxesVertexBuffer);
#pragma endregion

#pragma region Drawable Entities
    GameState->DrawableEntitiesVertexBuffer = {};
    GameState->DrawableEntitiesVertexBuffer.Size = QuadVerticesSize + GameState->EntityRenderInfoCount * sizeof(entity_render_info);
    GameState->DrawableEntitiesVertexBuffer.Usage = GL_STREAM_DRAW;

    GameState->DrawableEntitiesVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->DrawableEntitiesVertexBuffer.DataLayout->SubBufferCount = 2;
    GameState->DrawableEntitiesVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->DrawableEntitiesVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->DrawableEntitiesVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->DrawableEntitiesVertexBuffer.DataLayout->SubBuffers + 1;
        SubBuffer->Offset = QuadVerticesSize;
        SubBuffer->Size = GameState->EntityRenderInfoCount * sizeof(entity_render_info);
        SubBuffer->Data = GameState->EntityRenderInfos;
    }

    GameState->DrawableEntitiesVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->DrawableEntitiesVertexBuffer.AttributesLayout->AttributeCount = 7;
    GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->DrawableEntitiesVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 1;
        Attribute->Index = 1;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(entity_render_info);
        Attribute->Divisor = 1;
        // todo: really need to deal with this offset thing, man
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(entity_render_info, InstanceModel));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 2;
        Attribute->Index = 2;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(entity_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(entity_render_info, InstanceModel) + sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 3;
        Attribute->Index = 3;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(entity_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(entity_render_info, InstanceModel) + 2 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 4;
        Attribute->Index = 4;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(entity_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(entity_render_info, InstanceModel) + 3 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 5;
        Attribute->Index = 5;
        Attribute->Size = 2;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(entity_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(entity_render_info, InstanceUVOffset01));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 6;
        Attribute->Index = 6;
        Attribute->Size = 1;
        Attribute->Type = GL_UNSIGNED_INT;
        Attribute->Stride = sizeof(u32);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(entity_render_info, Flipped));
    }

    SetupVertexBuffer(Renderer, &GameState->DrawableEntitiesVertexBuffer);
#pragma endregion

#pragma region Particles
    GameState->ParticleRenderInfos = PushArray<particle_render_info>(&GameState->WorldArena, ArrayCount(GameState->Particles));

    GameState->ParticlesVertexBuffer = {};
    GameState->ParticlesVertexBuffer.Size = QuadVerticesSize + ArrayCount(GameState->Particles) * sizeof(particle_render_info);
    GameState->ParticlesVertexBuffer.Usage = GL_STREAM_DRAW;

    GameState->ParticlesVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->ParticlesVertexBuffer.DataLayout->SubBufferCount = 2;
    GameState->ParticlesVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->ParticlesVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->ParticlesVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->ParticlesVertexBuffer.DataLayout->SubBuffers + 1;
        SubBuffer->Offset = QuadVerticesSize;
        SubBuffer->Size = ArrayCount(GameState->Particles) * sizeof(particle_render_info);
        SubBuffer->Data = GameState->ParticleRenderInfos;
    }

    GameState->ParticlesVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->ParticlesVertexBuffer.AttributesLayout->AttributeCount = 6;
    GameState->ParticlesVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->ParticlesVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 1;
        Attribute->Index = 1;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        // todo: really need to deal with this offset thing, man
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(particle_render_info, Model));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 2;
        Attribute->Index = 2;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 3;
        Attribute->Index = 3;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + 2 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 4;
        Attribute->Index = 4;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + 3 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->ParticlesVertexBuffer.AttributesLayout->Attributes + 5;
        Attribute->Index = 5;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(particle_render_info, Color));
    }

    SetupVertexBuffer(Renderer, &GameState->ParticlesVertexBuffer);
#pragma endregion

#pragma region More Particles
    GameState->PlayerDiveParticleRenderInfos = PushArray<particle_render_info>
        (&GameState->WorldArena, ArrayCount(GameState->PlayerDiveParticles));

    GameState->PlayerDiveParticlesVertexBuffer = {};
    GameState->PlayerDiveParticlesVertexBuffer.Size = QuadVerticesSize + ArrayCount(GameState->PlayerDiveParticles) * sizeof(particle_render_info);
    GameState->PlayerDiveParticlesVertexBuffer.Usage = GL_STREAM_DRAW;

    GameState->PlayerDiveParticlesVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->PlayerDiveParticlesVertexBuffer.DataLayout->SubBufferCount = 2;
    GameState->PlayerDiveParticlesVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->PlayerDiveParticlesVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->PlayerDiveParticlesVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    {
        vertex_sub_buffer *SubBuffer = GameState->PlayerDiveParticlesVertexBuffer.DataLayout->SubBuffers + 1;
        SubBuffer->Offset = QuadVerticesSize;
        SubBuffer->Size = ArrayCount(GameState->PlayerDiveParticles) * sizeof(particle_render_info);
        SubBuffer->Data = GameState->PlayerDiveParticleRenderInfos;
    }

    GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->AttributeCount = 6;
    GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 1;
        Attribute->Index = 1;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        // todo: really need to deal with this offset thing, man
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(particle_render_info, Model));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 2;
        Attribute->Index = 2;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 3;
        Attribute->Index = 3;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + 2 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 4;
        Attribute->Index = 4;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)(QuadVerticesSize + StructOffset(particle_render_info, Model) + 3 * sizeof(vec4));
    }

    {
        vertex_buffer_attribute *Attribute = GameState->PlayerDiveParticlesVertexBuffer.AttributesLayout->Attributes + 5;
        Attribute->Index = 5;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(particle_render_info);
        Attribute->Divisor = 1;
        Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + StructOffset(particle_render_info, Color));
    }

    SetupVertexBuffer(Renderer, &GameState->PlayerDiveParticlesVertexBuffer);
#pragma endregion

#pragma region Quad

    GameState->QuadVertexBuffer = {};
    GameState->QuadVertexBuffer.Size = QuadVerticesSize;
    GameState->QuadVertexBuffer.Usage = GL_STATIC_DRAW;

    GameState->QuadVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
    GameState->QuadVertexBuffer.DataLayout->SubBufferCount = 1;
    GameState->QuadVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
        &GameState->WorldArena, GameState->QuadVertexBuffer.DataLayout->SubBufferCount);

    {
        vertex_sub_buffer *SubBuffer = GameState->QuadVertexBuffer.DataLayout->SubBuffers + 0;
        SubBuffer->Offset = 0;
        SubBuffer->Size = QuadVerticesSize;
        SubBuffer->Data = QuadVertices;
    }

    GameState->QuadVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
    GameState->QuadVertexBuffer.AttributesLayout->AttributeCount = 1;
    GameState->QuadVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
        &GameState->WorldArena, GameState->QuadVertexBuffer.AttributesLayout->AttributeCount);

    {
        vertex_buffer_attribute *Attribute = GameState->QuadVertexBuffer.AttributesLayout->Attributes + 0;
        Attribute->Index = 0;
        Attribute->Size = 4;
        Attribute->Type = GL_FLOAT;
        Attribute->Normalized = GL_FALSE;
        Attribute->Stride = sizeof(vec4);
        Attribute->Divisor = 0;
        Attribute->OffsetPointer = (void *)0;
    }

    SetupVertexBuffer(Renderer, &GameState->QuadVertexBuffer);
#pragma endregion

    GameState->UpdateRate = 10.f;   // 10 ms
    GameState->Lag = 0.f;
    GameState->Time = 0.f;

    GameState->Zoom = 1.f / 1.f;
    GameState->Camera = GameState->Player->Position;

    GameState->Entropy = RandomSequence(42);

    // init particles
    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
    {
        particle *Particle = GameState->Particles + ParticleIndex;

        Particle->RenderInfo = GameState->ParticleRenderInfos + ParticleIndex;
        Particle->RenderInfo->Offset = ParticleIndex * sizeof(particle_render_info) + QuadVerticesSize;
        Particle->RenderInfo->Model = mat4(1.f);
        Particle->RenderInfo->Color = vec4(0.f);
    }

    // more particles
    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->PlayerDiveParticles); ++ParticleIndex)
    {
        particle *Particle = GameState->PlayerDiveParticles + ParticleIndex;

        Particle->RenderInfo = GameState->PlayerDiveParticleRenderInfos + ParticleIndex;
        Particle->RenderInfo->Offset = ParticleIndex * sizeof(particle_render_info) + QuadVerticesSize;
        Particle->RenderInfo->Model = mat4(1.f);
        Particle->RenderInfo->Color = vec4(0.f);
    }

    Renderer->glEnable(GL_BLEND);
    Renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer->glEnable(GL_STENCIL_TEST);
    Renderer->glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    //Renderer->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GameState->IsInitialized = true;
}

extern "C" EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;

    platform_api *Platform = &Memory->Platform;
    renderer_api *Renderer = &Memory->Renderer;

    i32 ScreenWidth = Params->ScreenWidth;
    i32 ScreenHeight = Params->ScreenHeight;

    if (!GameState->IsInitialized)
    {
        GameInit(GameState, Memory, Params);
    }

    Renderer->glViewport(0, 0, ScreenWidth, ScreenHeight);

    GameState->Time += Params->msPerFrame;
    GameState->Lag += Params->msPerFrame;

    GameState->BackgroundColor = NormalizeRGB(29, 33, 45);

    ProcessInput(GameState, &Params->Input, Params->msPerFrame);

    while (GameState->Lag >= GameState->UpdateRate)
    {
        f32 dt = 0.1f;

        // friction imitation (todo: frame-rate dependent?)
        GameState->Player->Acceleration.x += -Params->msPerFrame * GameState->Player->Velocity.x;
        GameState->Player->Acceleration.y += -Params->msPerFrame * 0.01f * GameState->Player->Velocity.y;

        GameState->Player->Velocity += GameState->Player->Acceleration * dt;

        vec2 Move = 0.5f * GameState->Player->Acceleration * Square(dt) + GameState->Player->Velocity * dt;

        vec2 CollisionTime = vec2(1.f);

        for (u32 BoxIndex = 0; BoxIndex < GameState->TotalBoxCount; ++BoxIndex)
        {
            aabb *Box = GameState->Boxes + BoxIndex;

            for (u32 PlayerBoxIndex = 0; PlayerBoxIndex < GameState->Player->BoxCount; ++PlayerBoxIndex)
            {
                aabb_info *PlayerBox = GameState->Player->Boxes + PlayerBoxIndex;

                if (Box != PlayerBox->Box)
                {
                    vec2 t = SweptAABB(PlayerBox->Box->Position, Move, *Box, PlayerBox->Box->Size);

                    if (t.x >= 0.f && t.x < CollisionTime.x)
                    {
                        CollisionTime.x = t.x;
                    }
                    if (t.y >= 0.f && t.y < CollisionTime.y)
                    {
                        CollisionTime.y = t.y;
                    }
                }
            }
        }

        vec2 UpdatedMove = Move * CollisionTime;
        GameState->Player->Position.x += UpdatedMove.x;
        GameState->Player->Position.y += UpdatedMove.y;

        GameState->Player->Acceleration.x = 0.f;
        // gravity (todo: 9.8)
        GameState->Player->Acceleration.y = -0.4f;

        // collisions!
        if (CollisionTime.x < 1.f)
        {
            GameState->Player->Velocity.x = 0.f;
        }

        if (CollisionTime.y < 1.f)
        {
            GameState->Player->Velocity.y = 0.f;

            if (UpdatedMove.y < 0.f)
            {
                entity_state PlayerState = GetCurrentEntityState(GameState->Player);

                if (PlayerState == ENTITY_STATE_DIVE)
                {
                    EmitEvent(GameState, EVENT_TYPE_PLAYER_DIVE_HIT);
                }

                Pop(&GameState->Player->StatesStack);
                Push(&GameState->Player->StatesStack, ENTITY_STATE_SQUASH);
            }
        }

        entity_state PlayerState = GetCurrentEntityState(GameState->Player);

        if (GameState->Player->Velocity.y > 0.f)
        {
            if (PlayerState != ENTITY_STATE_JUMP)
            {
                if (PlayerState == ENTITY_STATE_FALL)
                {
                    Pop(&GameState->Player->StatesStack);
                }

                Push(&GameState->Player->StatesStack, ENTITY_STATE_JUMP);
            }
        }
        else if (GameState->Player->Velocity.y < 0.f)
        {
            if (PlayerState != ENTITY_STATE_FALL && PlayerState != ENTITY_STATE_DIVE)
            {
                if (PlayerState == ENTITY_STATE_JUMP || PlayerState == ENTITY_STATE_SQUASH)
                {
                    Pop(&GameState->Player->StatesStack);
                }
                Push(&GameState->Player->StatesStack, ENTITY_STATE_FALL);
            }
        }

        // todo: store 1/size as well
        GameState->Player->RenderInfo->InstanceModel = scale(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(1.f / GameState->Player->Size, 1.f)
        );
        GameState->Player->RenderInfo->InstanceModel = translate(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(UpdatedMove, 0.f)
        );
        GameState->Player->RenderInfo->InstanceModel = scale(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(GameState->Player->Size, 1.f)
        );

        for (u32 PlayerBoxModelIndex = 0; PlayerBoxModelIndex < GameState->Player->BoxCount; ++PlayerBoxModelIndex)
        {
            aabb_info *PlayerBox = GameState->Player->Boxes + PlayerBoxModelIndex;

            PlayerBox->Box->Position += UpdatedMove;

            *PlayerBox->Model = scale(*PlayerBox->Model, vec3(1.f / PlayerBox->Box->Size, 1.f));
            *PlayerBox->Model = translate(*PlayerBox->Model, vec3(UpdatedMove, 0.f));
            *PlayerBox->Model = scale(*PlayerBox->Model, vec3(PlayerBox->Box->Size, 1.f));
        }

        // camera
        // todo: y-idle as well
        vec2 IdleArea = {1.f, 1.f};

        if (UpdatedMove.x > 0.f)
        {
            if (GameState->Player->Position.x + GameState->Player->Size.x > GameState->Camera.x + IdleArea.x)
            {
                GameState->Camera.x += UpdatedMove.x;
            }
        }
        else if (UpdatedMove.x < 0.f)
        {
            if (GameState->Player->Position.x < GameState->Camera.x - IdleArea.x)
            {
                GameState->Camera.x += UpdatedMove.x;
            }
        }

        GameState->Camera.y += UpdatedMove.y;

        GameState->Projection = ortho(
            -GameState->ScreenWidthInWorldUnits / 2.f * GameState->Zoom, 
            GameState->ScreenWidthInWorldUnits / 2.f * GameState->Zoom,
            -GameState->ScreenHeightInWorldUnits / 2.f * GameState->Zoom,
            GameState->ScreenHeightInWorldUnits / 2.f * GameState->Zoom
        );

        mat4 View = mat4(1.f);
        View = translate(View, vec3(-GameState->Camera.x, -GameState->Camera.y, 0.f));
        View = translate(View, vec3(-GameState->ScreenWidthInWorldUnits / 2.f, -GameState->ScreenHeightInWorldUnits / 2.f, 0.f));

        GameState->VP = GameState->Projection * View;

        u32 ParticlesSpawn = 2;
        for (u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticlesSpawn; ++ParticleSpawnIndex)
        {
            particle *Particle = GameState->Particles + GameState->NextParticle++;

            if (GameState->NextParticle >= ArrayCount(GameState->Particles))
            {
                GameState->NextParticle = 0;
            }

            Particle->Position = vec2(
                RandomBetween(&GameState->Entropy, -0.1f, 0.1f) + 0.f, 
                RandomBetween(&GameState->Entropy, 0.f, 0.1f)
            );
            Particle->Velocity = vec2(
                RandomBetween(&GameState->Entropy, -0.5f, 0.5f), 
                RandomBetween(&GameState->Entropy, 2.f, 2.2f)
            );
            Particle->Acceleration = vec2(0.f, -1.5f);
            Particle->Color = vec4(
                RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                1.0f
            );
            Particle->dColor = vec4(0.f, 0.f, 0.f, -0.2f);
            Particle->Size = vec2(0.1f);
            Particle->dSize = vec2(-0.02f);
        }

        GameState->Lag -= GameState->UpdateRate;
    }

    // process events (todo: all of them?)
    while (GameState->EventQueue.Head != GameState->EventQueue.Tail)
    {
        event Event = Dequeue(&GameState->EventQueue);

        switch (Event.Type)
        {
            case EVENT_TYPE_PLAYER_DIVE_HIT:
            {
                u32 ParticlesSpawn = 20;
                for (u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticlesSpawn; ++ParticleSpawnIndex)
                {
                    particle *Particle = GameState->PlayerDiveParticles + GameState->NextPlayerDiveParticle++;

                    if (GameState->NextPlayerDiveParticle >= ArrayCount(GameState->PlayerDiveParticles))
                    {
                        GameState->NextPlayerDiveParticle = 0;
                    }

                    // todo: clean this mess with coordinates
                    Particle->Position = vec2(
                        RandomBetween(&GameState->Entropy, -0.1f, 0.1f) + 0.f, 
                        RandomBetween(&GameState->Entropy, 0.f, 0.1f)
                    ) + vec2(GameState->Player->Position.x, GameState->Player->Position.y);
                    Particle->Velocity = vec2(
                        RandomBetween(&GameState->Entropy, -0.5f, 0.5f), 
                        RandomBetween(&GameState->Entropy, 4.f, 4.2f)
                    );
                    Particle->Acceleration = vec2(0.f, -6.5f);
                    Particle->Color = vec4(
                        RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                        RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                        RandomBetween(&GameState->Entropy, 0.75f, 1.0f),
                        1.0f
                    );
                    Particle->dColor = vec4(0.f, 0.f, 0.f, -0.6f);
                    Particle->Size = vec2(0.1f);
                    Particle->dSize = vec2(-0.02f);
                }
            }
            break;

            InvalidDefaultCase;
        }
    }

    Renderer->glClearColor(GameState->BackgroundColor.r, GameState->BackgroundColor.g, GameState->BackgroundColor.b, 1.f);
    Renderer->glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    Renderer->glBindTexture(GL_TEXTURE_2D, GameState->TilesetTexture);

    Renderer->glBindBuffer(GL_UNIFORM_BUFFER, GameState->UBO);
    Renderer->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &GameState->VP);

    Renderer->glStencilFunc(GL_ALWAYS, 1, 0x00);
    Renderer->glStencilMask(0x00);

    // Draw tiles
    Renderer->glUseProgram(GameState->TilesShaderProgram.ProgramHandle);
    Renderer->glBindVertexArray(GameState->TilesVertexBuffer.VAO);
    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalTileCount);

    // Draw entities (with borders)
    // 1st render pass: draw objects as normal, writing to the stencil buffer
    Renderer->glStencilFunc(GL_ALWAYS, 1, 0xFF);
    Renderer->glStencilMask(0xFF);

    Renderer->glUseProgram(GameState->DrawableEntitiesShaderProgram.ProgramHandle);
    Renderer->glBindVertexArray(GameState->DrawableEntitiesVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->DrawableEntitiesVertexBuffer.VBO);

    // mapping entity state to animation
    entity_state PlayerState = GetCurrentEntityState(GameState->Player);
    animation *PlayerAnimation = GameState->Player->CurrentAnimation;

    switch (PlayerState)
    {
        case ENTITY_STATE_IDLE:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_IDLE");
            break;
        case ENTITY_STATE_RUN:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_RUN");
            break;
        case ENTITY_STATE_JUMP:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_JUMP");
            break;
        case ENTITY_STATE_DIVE:
            // todo: different animation for dive?
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_FALL");
            break;
        case ENTITY_STATE_FALL:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_FALL");
            break;
        case ENTITY_STATE_SQUASH:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_SQUASH");
            break;
        case ENTITY_STATE_ATTACK:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_ATTACK");
            break;
        case ENTITY_STATE_DUCK:
            ChangeAnimationIfDifferent(GameState, GameState->Player, "PLAYER_DUCK");
            break;

        InvalidDefaultCase;
    }

    // animations
    for (u32 EntityIndex = 0; EntityIndex < GameState->TotalDrawableObjectCount; ++EntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + EntityIndex;

        if (Entity->CurrentAnimation)
        {
            animation *Animation = Entity->CurrentAnimation;
            animation_frame *CurrentFrame = GetCurrentAnimationFrame(Animation);

            if (Animation->CurrentTime >= CurrentFrame->Duration)
            {
                Animation->CurrentFrameIndex++;

                if (Animation->CurrentFrameIndex >= Animation->AnimationFrameCount)
                {
                    // todo:
                    entity_state EntityState = GetCurrentEntityState(Entity);

                    switch (EntityState)
                    {
                    case ENTITY_STATE_ATTACK:
                        Pop(&Entity->StatesStack);
                        break;
                    case ENTITY_STATE_SQUASH:
                        Pop(&Entity->StatesStack);
                        break;
                    }

                    if (Animation->StopOnTheLastFrame)
                    {
                        Animation->CurrentFrameIndex--;
                    }
                    else
                    {
                        if (Entity->CurrentAnimation->NextToPlay)
                        {
                            Animation = Entity->CurrentAnimation->NextToPlay;
                            ChangeAnimation(GameState, Entity, Animation);
                        }
                        else
                        {
                            Entity->CurrentAnimation = nullptr;
                        }
                    }
                }

                CurrentFrame = GetCurrentAnimationFrame(Animation);

                CurrentFrame->CurrentXOffset01 = CurrentFrame->XOffset01;
                CurrentFrame->CurrentYOffset01 = CurrentFrame->YOffset01;

                Animation->CurrentTime = 0.f;
           }

            Entity->RenderInfo->InstanceUVOffset01.x = CurrentFrame->CurrentXOffset01;
            Entity->RenderInfo->InstanceUVOffset01.y = CurrentFrame->CurrentYOffset01;

            Animation->CurrentTime += Params->msPerFrame;
        }
    }

    for (u32 EntityIndex = 0; EntityIndex < GameState->TotalDrawableObjectCount; ++EntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + EntityIndex;

        Renderer->glBufferSubData(GL_ARRAY_BUFFER, Entity->RenderInfo->Offset, sizeof(entity_render_info), Entity->RenderInfo);
    }

    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player->RenderInfo->Offset, sizeof(entity_render_info), GameState->Player->RenderInfo);

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalDrawableObjectCount);

    // 2nd render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
    Renderer->glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    Renderer->glStencilMask(0x00);

    Renderer->glUseProgram(GameState->DrawableEntitiesBorderShaderProgram.ProgramHandle);

    {
        shader_uniform *ColorUniform = GetUniform(&GameState->DrawableEntitiesBorderShaderProgram, "u_Color");
        
        vec4 Color = vec4(0.f, 0.f, 1.f, 1.f);
        SetShaderUniform(Renderer, ColorUniform->Location, Color);
    }

    f32 scaleFactor = 1.1f;

    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->TotalDrawableObjectCount; ++DrawableEntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + DrawableEntityIndex;

        Entity->RenderInfo->InstanceModel = translate(Entity->RenderInfo->InstanceModel, vec3(Entity->Size / 2.f, 1.f));
        Entity->RenderInfo->InstanceModel = scale(Entity->RenderInfo->InstanceModel, vec3(scaleFactor, scaleFactor, 1.f));
        Entity->RenderInfo->InstanceModel = translate(Entity->RenderInfo->InstanceModel, vec3(-Entity->Size / 2.f, 1.f));

        Renderer->glBufferSubData(
            GL_ARRAY_BUFFER, Entity->RenderInfo->Offset + StructOffset(entity_render_info, InstanceModel), 
            sizeof(mat4), &Entity->RenderInfo->InstanceModel
        );
    }

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalDrawableObjectCount);

    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->TotalDrawableObjectCount; ++DrawableEntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + DrawableEntityIndex;

        Entity->RenderInfo->InstanceModel = translate(Entity->RenderInfo->InstanceModel, vec3(Entity->Size / 2.f, 1.f));
        Entity->RenderInfo->InstanceModel = scale(Entity->RenderInfo->InstanceModel, vec3(1.f / scaleFactor, 1.f / scaleFactor, 1.f));
        Entity->RenderInfo->InstanceModel = translate(Entity->RenderInfo->InstanceModel, vec3(-Entity->Size / 2.f, 1.f));

        Renderer->glBufferSubData(
            GL_ARRAY_BUFFER, Entity->RenderInfo->Offset + StructOffset(entity_render_info, InstanceModel),
            sizeof(mat4), &Entity->RenderInfo->InstanceModel
        );
    }

    Renderer->glStencilFunc(GL_ALWAYS, 1, 0xFF);
    Renderer->glStencilMask(0xFF);

    // Draw collidable regions (todo: update them as well, not only player's boxes)
    Renderer->glUseProgram(GameState->BoxesShaderProgram.ProgramHandle);
    Renderer->glBindVertexArray(GameState->BoxesVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->BoxesVertexBuffer.VBO);

    for (u32 PlayerBoxModelIndex = 0; PlayerBoxModelIndex < GameState->Player->BoxCount; ++PlayerBoxModelIndex)
    {
        aabb_info *PlayerBox = GameState->Player->Boxes + PlayerBoxModelIndex;

        Renderer->glBufferSubData(
            GL_ARRAY_BUFFER, GameState->Player->RenderInfo->BoxModelOffset + PlayerBoxModelIndex * sizeof(mat4), 
            sizeof(mat4), PlayerBox->Model
        );
    }

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalBoxCount);

    // draw some borders
    {
        vec2 Position = vec2(GameState->Camera.x, GameState->Camera.y);
        vec2 Size = vec2(GameState->ScreenWidthInWorldUnits, GameState->ScreenHeightInWorldUnits);
        rotation_info Rotation = {};
        Rotation.AngleInRadians = 0.f;
        Rotation.Axis = vec3(0.f, 0.f, 1.f);
        f32 Thickness = 0.2f;
        vec4 Color = vec4(0.f, 1.f, 0.f, 1.f);

        DrawRectangleOutline(Renderer, GameState, Position, Size, &Rotation, Thickness, Color);
    }

    {
        // todo: consolidate about game world coordinates ([0,0] is at the center)
        vec2 Position = vec2(GameState->ScreenWidthInWorldUnits / 2.f, GameState->ScreenHeightInWorldUnits / 2.f) + vec2(6.f, 1.f);
        vec2 Size = vec2(1.f, 1.f);
        rotation_info Rotation = {};
        Rotation.AngleInRadians = radians((f32)GameState->Time * 0.2f);
        Rotation.Axis = vec3(0.f, 1.f, 0.f);
        f32 Thickness = 0.1f;
        vec4 Color = vec4(1.f, 1.f, 0.f, 1.f);

        DrawRectangleOutline(Renderer, GameState, Position, Size, &Rotation, Thickness, Color);
    }

    f32 dt = Params->msPerFrame * 0.001f;

    Renderer->glUseProgram(GameState->ParticlesShaderProgram.ProgramHandle);
    Renderer->glBindVertexArray(GameState->ParticlesVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->ParticlesVertexBuffer.VBO);

    // todo: test code
    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
    {
        particle *Particle = GameState->Particles + ParticleIndex;

        if (Particle->Color.a <= 0.f) {
            continue;
        }

        Particle->Position += 0.5f * Particle->Acceleration * Square(dt) + Particle->Velocity * dt;
        Particle->Velocity += dt * Particle->Acceleration;
        Particle->Color += dt * Particle->dColor;
        Particle->Size += dt * Particle->dSize;

        f32 CoefficientOfRestitution = 0.3f;

        if (Particle->Position.y < 0.f)
        {
            Particle->Position.y = -Particle->Position.y;
            Particle->Velocity.y = -Particle->Velocity.y * CoefficientOfRestitution;
        }

        {
            vec2 Position = vec2(GameState->ScreenWidthInWorldUnits / 2.f, GameState->ScreenHeightInWorldUnits / 2.f) + Particle->Position;
            f32 Rotation = (f32)radians(GameState->Time * 250.f);
            vec4 Color = Particle->Color;
            vec2 Size = Particle->Size;

            Particle->RenderInfo->Model = mat4(1.f);
            // translation
            Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(Position, 0.f));
            // scaling
            Particle->RenderInfo->Model = scale(Particle->RenderInfo->Model, vec3(Size, 0.f));
            // rotation
            //Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(Size / 2.f, 0.f));
            //Particle->RenderInfo->Model = rotate(Particle->RenderInfo->Model, Rotation, vec3(0.f, 0.f, 1.f));
            //Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(-Size / 2.f, 0.f));3


            Particle->RenderInfo->Color = Color;
        }
    }

    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->QuadVerticesSize, 
        ArrayCount(GameState->Particles) * sizeof(particle_render_info), GameState->ParticleRenderInfos);
    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ArrayCount(GameState->Particles));

    // more particles
    Renderer->glBindVertexArray(GameState->PlayerDiveParticlesVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->PlayerDiveParticlesVertexBuffer.VBO);

    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->PlayerDiveParticles); ++ParticleIndex)
    {
        particle *Particle = GameState->PlayerDiveParticles + ParticleIndex;

        if (Particle->Color.a <= 0.f) {
            continue;
        }

        Particle->Position += 0.5f * Particle->Acceleration * Square(dt) + Particle->Velocity * dt;
        Particle->Velocity += dt * Particle->Acceleration;
        Particle->Color += dt * Particle->dColor;
        Particle->Size += dt * Particle->dSize;

        f32 CoefficientOfRestitution = 0.3f;

        //if (Particle->Position.y < -GameState->Player->Position.y)
        //{
        //    Particle->Position.y = -Particle->Position.y;
        //    Particle->Velocity.y = -Particle->Velocity.y * CoefficientOfRestitution;
        //}

        {
            vec2 Position = vec2(GameState->ScreenWidthInWorldUnits / 2.f, GameState->ScreenHeightInWorldUnits / 2.f) + Particle->Position;
            f32 Rotation = (f32)radians(GameState->Time);
            vec4 Color = Particle->Color;
            vec2 Size = Particle->Size;

            Particle->RenderInfo->Model = mat4(1.f);
            // translation
            Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(Position, 0.f));
            // scaling
            Particle->RenderInfo->Model = scale(Particle->RenderInfo->Model, vec3(Size, 0.f));
            // rotation
            //Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(Size / 2.f, 0.f));
            //Particle->RenderInfo->Model = rotate(Particle->RenderInfo->Model, Rotation, vec3(0.f, 0.f, 1.f));
            //Particle->RenderInfo->Model = translate(Particle->RenderInfo->Model, vec3(-Size / 2.f, 0.f));

            Particle->RenderInfo->Color = Color;
        }
    }

    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->QuadVerticesSize, 
        ArrayCount(GameState->PlayerDiveParticles) * sizeof(particle_render_info), GameState->PlayerDiveParticleRenderInfos);
    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ArrayCount(GameState->PlayerDiveParticles));

    // draw some test sprites
    {
        vec2 Size = vec2(1.f);
        vec2 Position = vec2(0.f, 0.f);
        rotation_info Rotation = {};
        Rotation.AngleInRadians = (f32)radians(-90.f);
        Rotation.Axis = vec3(0.f, 0.f, 1.f);
        vec2 Alignment = vec2(0.5f, 0.5f);
        vec2 UV = GetUVOffset01FromTileID(&GameState->Map.Tilesets[0].Source, 325);

        DrawSprite(Renderer, GameState, Position, Size, &Rotation, UV, Alignment);
    }

    // draw text
    mat4 TextProjection = ortho(
        -GameState->ScreenWidthInWorldUnits / 2.f, 
        GameState->ScreenWidthInWorldUnits / 2.f,
        -GameState->ScreenHeightInWorldUnits / 2.f,
        GameState->ScreenHeightInWorldUnits / 2.f
    );

    Renderer->glBindBuffer(GL_UNIFORM_BUFFER, GameState->UBO);
    Renderer->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &TextProjection);

    Renderer->glBindTexture(GL_TEXTURE_2D, GameState->FontTextureAtlas);

    {
        vec2 Position = vec2(0.5f + -GameState->ScreenWidthInWorldUnits / 2.f, -1.f + GameState->ScreenHeightInWorldUnits / 2.f);
        //vec2 Position = vec2(0.f, 0.f);
        rotation_info Rotation = {};
        Rotation.AngleInRadians = 0.f;
        Rotation.Axis = vec3(0.f, 1.f, 0.f);
        f32 TextScale = 0.5f;

        wchar FrameFps[32];
        FormatString(FrameFps, ArrayCount(FrameFps), L"fps: %.1f", 1000.f / Params->msPerFrame);

        wchar FrameTime[32];
        FormatString(FrameTime, ArrayCount(FrameTime), L"ms: %.4f", Params->msPerFrame);

        char PlayerStateString[32];
        GetEntityStateString(GetCurrentEntityState(GameState->Player), PlayerStateString, ArrayCount(PlayerStateString));

        wchar PlayerState[32];
        FormatString(PlayerState, ArrayCount(PlayerState), L"player state: %S, count: %u", PlayerStateString, GameState->Player->StatesStack.Head);

        wchar PlayerPosition[64];
        FormatString(PlayerPosition, ArrayCount(PlayerPosition), L"player position: x: %.2f, y: %.2f", GameState->Player->Position.x, GameState->Player->Position.y);

        wchar MousePosition[64];
        f32 CanonicalMouseX = (Params->Input.MouseX * GameState->PixelsToWorldUnits - 
            GameState->ScreenWidthInWorldUnits / 2.f + GameState->Camera.x);
        f32 CanonicalMouseY = (Params->Input.MouseY * GameState->PixelsToWorldUnits - 
            GameState->ScreenHeightInWorldUnits / 2.f + GameState->Camera.y);

        FormatString(MousePosition, ArrayCount(MousePosition), L"mouse position: x: %.2f, y: %.2f", CanonicalMouseX, CanonicalMouseY);

        f32 NextLineAdvance = GameState->CurrentFont->VerticalAdvance * GameState->PixelsToWorldUnits * TextScale;
        DrawTextLine(Renderer, GameState, FrameFps, Position, TextScale, &Rotation, vec4(0.f, 1.f, 1.f, 1.f), GameState->CurrentFont);
        DrawTextLine(Renderer, GameState, FrameTime, Position - vec2(0.f, 1.f * NextLineAdvance), TextScale, &Rotation, vec4(0.f, 1.f, 1.f, 1.f), GameState->CurrentFont);
        DrawTextLine(Renderer, GameState, PlayerState, Position - vec2(0.f, 2.f * NextLineAdvance), TextScale, &Rotation, vec4(0.f, 1.f, 1.f, 1.f), GameState->CurrentFont);
        DrawTextLine(Renderer, GameState, PlayerPosition, Position - vec2(0.f, 3.f * NextLineAdvance), TextScale, &Rotation, vec4(0.f, 1.f, 1.f, 1.f), GameState->CurrentFont);
        DrawTextLine(Renderer, GameState, MousePosition, Position - vec2(0.f, 4.f * NextLineAdvance), TextScale, &Rotation, vec4(0.f, 1.f, 1.f, 1.f), GameState->CurrentFont);
    }
}