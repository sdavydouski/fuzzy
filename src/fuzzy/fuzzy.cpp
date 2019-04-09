// todo: for defines and such - won't be needed in future
#include "glad/glad.h"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>

#include "fuzzy_types.h"
#include "fuzzy_platform.h"
#include "tiled.cpp"
#include "fuzzy_graphics.cpp"
#include "fuzzy.h"

global_variable const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
global_variable const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
global_variable const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

internal_function inline vec3
NormalizeRGB(u32 Red, u32 Green, u32 Blue)
{
    const f32 MAX = 255.f;
    return vec3(Red / MAX, Green / MAX, Blue / MAX);
}

// todo: write more efficient functions
internal_function inline f32
Clamp(f32 Value, f32 Min, f32 Max)
{
    if (Value < Min) return Min;
    if (Value > Max) return Max;

    return Value;
}

//inline f32 abs(f32 value) {
//    if (value < 0.f) return -value;
//
//    return value;
//}

internal_function inline f32
GetRandomInRange(f32 Min, f32 Max)
{
    f32 Result = Min + (f32)(rand()) / ((f32)(RAND_MAX / (Max - Min)));
    return Result;
}

internal_function inline b32
IntersectAABB(const aabb& Box1, const aabb& Box2)
{
    // Separating Axis Theorem
    b32 XCollision = Box1.Position.x + Box1.Size.x > Box2.Position.x && Box1.Position.x < Box2.Position.x + Box2.Size.x;
    b32 YCollision = Box1.Position.y + Box1.Size.y > Box2.Position.y && Box1.Position.y < Box2.Position.y + Box2.Size.y;

    return XCollision && YCollision;
}

// basic Minkowski-based collision detection
internal_function vec2
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

global_variable const vec3 BackgroundColor = NormalizeRGB(29, 33, 45);

internal_function inline animation_frame *
GetCurrentAnimationFrame(animation *Animation)
{
    animation_frame *Result = Animation->AnimationFrames + Animation->CurrentFrameIndex;
    return Result;
}

internal_function inline animation *
GetAnimation(game_state *GameState, animation_type Type)
{
    animation *Result = GameState->Animations + Type;
    return Result;
}

internal_function void
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

internal_function void
ChangeAnimation(game_state *GameState, entity *Entity, animation_type Type, b32 Loop = true)
{
    animation *Animation = GetAnimation(GameState, Type);
    ChangeAnimation(GameState, Entity, Animation, Loop);
}

internal_function void
ProcessInput(game_state *GameState, game_input *Input, f32 Delta)
{
    if (Input->Left.isPressed)
    {
        GameState->Player->Acceleration.x = -8.f;

        // todo: in future handle flipped vertically/diagonally
        GameState->Player->RenderInfo->Flipped = true;

        if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_RUN) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_SQUASH) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_UP) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_DOWN))
        {
            ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_RUN);
        }
    }

    if (Input->Right.isPressed)
    {
        GameState->Player->Acceleration.x = 8.f;

        GameState->Player->RenderInfo->Flipped = false;

        if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_RUN) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_SQUASH) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_UP) &&
            GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_DOWN))
        {
            ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_RUN);
        }
    }

    if (!Input->Left.isPressed && !Input->Left.isProcessed)
    {
        Input->Left.isProcessed = true;
        if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_IDLE))
        {
            ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_IDLE);
        }
    }

    if (!Input->Right.isPressed && !Input->Right.isProcessed)
    {
        Input->Right.isProcessed = true;
        if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_IDLE))
        {
            ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_IDLE);
        }
    }

    if (Input->Jump.isPressed && !Input->Jump.isProcessed)
    {
        Input->Jump.isProcessed = true;
        GameState->Player->Acceleration.y = 12.f;
        GameState->Player->Velocity.y = 0.f;
    }

    f32 ZoomScale = 0.2f;

    // todo: float precision!
    if (Input->ScrollY == 0.f)
    {
        GameState->Zoom = 1.f;
    }
    else if (Input->ScrollY > 0.f)
    {
        GameState->Zoom = 1.f / (1.f + Input->ScrollY * ZoomScale);
    }
    else if (Input->ScrollY < 0.f)
    {
        GameState->Zoom = 1.f + abs(Input->ScrollY) * ZoomScale;
    }

    //GameState->Zoom = Clamp(GameState->Zoom, 0.1f, 4.f);

    //if (Input->Keys[KEY_LEFT] == KEY_PRESS) {
    //    if (
    //        Bob->CurrentAnimation != Bob->Animations[2] &&
    //        Bob->CurrentAnimation != Bob->Animations[1] &&
    //        Bob->CurrentAnimation != Bob->Animations[3]
    //        ) {
    //        Bob->CurrentAnimation = Bob->Animations[2];
    //    }
    //    Bob->Acceleration.x = -12.f;
    //    Bob->Flipped |= FLIPPED_HORIZONTALLY_FLAG;
    //}

    //if (Input->Keys[KEY_RIGHT] == KEY_PRESS) {
    //    if (
    //        Bob->CurrentAnimation != Bob->Animations[2] &&
    //        Bob->CurrentAnimation != Bob->Animations[1] &&
    //        Bob->CurrentAnimation != Bob->Animations[3]
    //        ) {
    //        Bob->CurrentAnimation = Bob->Animations[2];
    //    }
    //    Bob->Acceleration.x = 12.f;
    //    Bob->Flipped &= 0;
    //}

    //if (Input->Keys[KEY_LEFT] == KEY_RELEASE && !Input->ProcessedKeys[KEY_LEFT]) {
    //    Input->ProcessedKeys[KEY_LEFT] = true;
    //    if (Bob->CurrentAnimation != Bob->Animations[0]) {
    //        Bob->CurrentAnimation = Bob->Animations[0];
    //        Bob->XAnimationOffset = 0.f;
    //        Bob->Flipped |= FLIPPED_HORIZONTALLY_FLAG;
    //    }
    //}

    //if (Input->Keys[KEY_RIGHT] == KEY_RELEASE && !Input->ProcessedKeys[KEY_RIGHT]) {
    //    Input->ProcessedKeys[KEY_RIGHT] = true;
    //    if (Bob->CurrentAnimation != Bob->Animations[0]) {
    //        Bob->CurrentAnimation = Bob->Animations[0];
    //        Bob->XAnimationOffset = 0.f;
    //        Bob->Flipped &= 0;
    //    }
    //}

    //if (Input->Keys[KEY_SPACE] == KEY_PRESS && !Input->ProcessedKeys[KEY_SPACE]) {
    //    Input->ProcessedKeys[KEY_SPACE] = true;
    //    Bob->Acceleration.y = -350.f;
    //    Bob->Velocity.y = 0.f;
    //}

    //if (Input->Keys[KEY_S] == KEY_PRESS && !Input->ProcessedKeys[KEY_S]) {
    //    Input->ProcessedKeys[KEY_S] = true;
    //    Bob->CurrentAnimation = Bob->Animations[3];
    //    Bob->XAnimationOffset = 0.f;

    //    Swoosh->ShouldRender = true;
    //    Swoosh->XAnimationOffset = 0.f;
    //    Swoosh->Flipped = Bob->Flipped;

    //    // todo: make it better
    //    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
    //        drawable_entity *Entity = &GameState->DrawableEntities[DrawableEntityIndex];
    //        if (Entity->Type == tile_type::REFLECTOR) {
    //            Entity->UnderEffect = false;
    //        }
    //    }

    //    if (Swoosh->Flipped & FLIPPED_HORIZONTALLY_FLAG) {
    //        Swoosh->Position = { Bob->Box.Position.x - 2 * TILE_SIZE.x, Bob->Box.Position.y };
    //        Swoosh->Box.Position = { Bob->Box.Position.x - 2 * TILE_SIZE.x, Bob->Box.Position.y };
    //    }
    //    else {
    //        Swoosh->Position = { Bob->Box.Position.x + TILE_SIZE.x, Bob->Box.Position.y };
    //        Swoosh->Box.Position = { Bob->Box.Position.x + TILE_SIZE.x, Bob->Box.Position.y };
    //    }
    //}
}

internal_function animation_type
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

extern "C" EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;

    s32 ScreenWidth = Params->ScreenWidth;
    s32 ScreenHeight = Params->ScreenHeight;
    vec2 ScreenCenter = vec2(ScreenWidth / 2.f, ScreenHeight / 2.f);

    platform_api *Platform = &Memory->Platform;
    renderer_api *Renderer = &Memory->Renderer;

    if (!GameState->IsInitialized)
    {
        InitializeMemoryArena(
            &GameState->WorldArena,
            Memory->PermanentStorageSize - sizeof(game_state),
            (u8*)Memory->PermanentStorage + sizeof(game_state)
        );

        GameState->ScreenWidthInMeters = 20.f;
        f32 MetersToPixels = (f32)ScreenWidth / GameState->ScreenWidthInMeters;
        f32 PixelsToMeters = 1.f / MetersToPixels;
        GameState->ScreenHeightInMeters = ScreenHeight * PixelsToMeters;

        char *MapJson = (char*)Platform->ReadFile("maps/map01.json").Contents;
        GameState->Map = {};
        LoadMap(&GameState->Map, MapJson, &GameState->WorldArena, Platform);

        tile_meta_info *TileInfo = GetTileMetaInfo(&GameState->Map.Tilesets[0].Source, 544);

        tileset *Tileset = &GameState->Map.Tilesets[0].Source;
        u32 TilesetFirstGID = GameState->Map.Tilesets[0].FirstGID;

        char *OpenGLVersion = (char*)Renderer->glGetString(GL_VERSION);
        Platform->PrintOutput(OpenGLVersion);
        Platform->PrintOutput("\n");

        u32 texture;
        Renderer->glGenTextures(1, &texture);
        Renderer->glBindTexture(GL_TEXTURE_2D, texture);

        // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
        // since we do not use mipmaps we must override this value
        Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        Renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Tileset->Image.Width, Tileset->Image.Height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, Tileset->Image.Memory);

        //Platform->FreeImageFile(textureImage);
        vec2 TileSize01 = vec2((f32)Tileset->TileWidthInPixels / (f32)Tileset->Image.Width,
            (f32)Tileset->TileHeightInPixels / (f32)Tileset->Image.Height);

        const u32 transformsBindingPoint = 0;

        {
            GameState->TilesShaderProgram = CreateShaderProgram(Memory, GameState, "shaders/tile.vert", "shaders/tile.frag");

            u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->TilesShaderProgram, "transforms");
            Renderer->glUniformBlockBinding(GameState->TilesShaderProgram, transformsUniformBlockIndex, transformsBindingPoint);

            Renderer->glUseProgram(GameState->TilesShaderProgram);

            s32 TileSizeUniformLocation = GetUniformLocation(Memory, GameState->TilesShaderProgram, "u_TileSize");
            SetShaderUniform(Memory, TileSizeUniformLocation, TileSize01);
        }

        {
            GameState->BoxesShaderProgram = CreateShaderProgram(Memory, GameState, "shaders/box.vert", "shaders/box.frag");

            u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->BoxesShaderProgram, "transforms");
            Renderer->glUniformBlockBinding(GameState->BoxesShaderProgram, transformsUniformBlockIndex, transformsBindingPoint);
        }

        {
            GameState->DrawableEntitiesShaderProgram = CreateShaderProgram(Memory, GameState, "shaders/entity.vert", "shaders/entity.frag");;

            u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->DrawableEntitiesShaderProgram, "transforms");
            Renderer->glUniformBlockBinding(GameState->DrawableEntitiesShaderProgram, transformsUniformBlockIndex, transformsBindingPoint);

            Renderer->glUseProgram(GameState->DrawableEntitiesShaderProgram);

            s32 TileSizeUniformLocation = GetUniformLocation(Memory, GameState->DrawableEntitiesShaderProgram, "u_TileSize");
            SetShaderUniform(Memory, TileSizeUniformLocation, TileSize01);
        }

        {
            // todo: use different fragment shader
            GameState->DrawableEntitiesBorderShaderProgram = CreateShaderProgram(Memory, GameState, "shaders/entity.vert", "shaders/border.frag");;

            u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->DrawableEntitiesBorderShaderProgram, "transforms");
            Renderer->glUniformBlockBinding(GameState->DrawableEntitiesBorderShaderProgram, transformsUniformBlockIndex, transformsBindingPoint);

            Renderer->glUseProgram(GameState->DrawableEntitiesBorderShaderProgram);

            s32 TileSizeUniformLocation = GetUniformLocation(Memory, GameState->DrawableEntitiesBorderShaderProgram, "u_TileSize");
            SetShaderUniform(Memory, TileSizeUniformLocation, TileSize01);
        }

        {
            GameState->BorderShaderProgram = CreateShaderProgram(Memory, GameState, "shaders/border.vert", "shaders/border.frag");;

            u32 transformsUniformBlockIndex = Renderer->glGetUniformBlockIndex(GameState->DrawableEntitiesBorderShaderProgram, "transforms");
            Renderer->glUniformBlockBinding(GameState->BorderShaderProgram, transformsUniformBlockIndex, transformsBindingPoint);
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
            for (u32 ChunkIndex = 0; ChunkIndex < GameState->Map.TileLayers[TileLayerIndex].ChunkCount; ++ChunkIndex)
            {
                for (u32 GIDIndex = 0; GIDIndex < GameState->Map.TileLayers[TileLayerIndex].Chunks[ChunkIndex].GIDCount; ++GIDIndex)
                {
                    u32 GID = GameState->Map.TileLayers[TileLayerIndex].Chunks[ChunkIndex].GIDs[GIDIndex];
                    if (GID > 0)
                    {
                        ++GameState->TotalTileCount;

                        tile_meta_info *TileInfo = GetTileMetaInfo(Tileset, GID - TilesetFirstGID);
                        if (TileInfo)
                        {
                            GameState->TotalBoxCount += TileInfo->BoxCount;
                        }
                    }
                }
            }
        }

        GameState->TotalObjectCount = 0;
        GameState->TotalDrawableObjectCount = 0;
        for (u32 ObjectLayerIndex = 0; ObjectLayerIndex < GameState->Map.ObjectLayerCount; ++ObjectLayerIndex)
        {
            u32 ObjectCount = GameState->Map.ObjectLayers[ObjectLayerIndex].ObjectCount;
            GameState->TotalObjectCount += ObjectCount;

            for (u32 ObjectIndex = 0; ObjectIndex < ObjectCount; ++ObjectIndex)
            {
                map_object *Object = GameState->Map.ObjectLayers[ObjectLayerIndex].Objects + ObjectIndex;

                if (Object->GID)
                {
                    ++GameState->TotalDrawableObjectCount;

                    u32 TileID = Object->GID - TilesetFirstGID;
                    tile_meta_info *TileInfo = GetTileMetaInfo(Tileset, TileID);

                    if (TileInfo)
                    {
                        GameState->TotalBoxCount += TileInfo->BoxCount;
                    }
                }
            }
        }

        GameState->AnimationCount = 0;

        for (u32 TilesetIndex = 0; TilesetIndex < GameState->Map.TilesetCount; ++TilesetIndex)
        {
            tileset_source *TilesetSource = GameState->Map.Tilesets + TilesetIndex;
            tileset Tileset = TilesetSource->Source;

            for (u32 TileIndex = 0; TileIndex < Tileset.TileCount; ++TileIndex)
            {
                tile_meta_info *Tile = Tileset.Tiles + TileIndex;
                
                if (Tile->AnimationFrameCount > 0)
                {
                    ++GameState->AnimationCount;
                }
            }
        }

        GameState->Animations = PushArray<animation>(&GameState->WorldArena, GameState->AnimationCount);

        for (u32 TilesetIndex = 0; TilesetIndex < GameState->Map.TilesetCount; ++TilesetIndex)
        {
            tileset_source *TilesetSource = GameState->Map.Tilesets + TilesetIndex;
            tileset Tileset = TilesetSource->Source;

            for (u32 TileIndex = 0; TileIndex < Tileset.TileCount; ++TileIndex)
            {
                tile_meta_info *Tile = Tileset.Tiles + TileIndex;

                if (Tile->AnimationFrameCount > 0)
                {
                    char *AnimationTypeString = nullptr;

                    for (u32 CustomPropertyIndex = 0; CustomPropertyIndex < Tile->CustomPropertiesCount; ++CustomPropertyIndex)
                    {
                        tile_custom_property *CustomProperty = Tile->CustomProperties + CustomPropertyIndex;

                        if (StringEquals(CustomProperty->Name, "AnimationType"))
                        {
                            AnimationTypeString = (char *)CustomProperty->Value;
                            break;
                        }
                    }

                    assert(AnimationTypeString);

                    animation_type AnimationType = GetAnimationTypeFromString(AnimationTypeString);
                    animation *Animation = GameState->Animations + AnimationType;

                    *Animation = {};
                    Animation->CurrentFrameIndex = 0;
                    Animation->AnimationFrameCount = Tile->AnimationFrameCount;
                    Animation->AnimationFrames = PushArray<animation_frame>(&GameState->WorldArena, Animation->AnimationFrameCount);

                    for (u32 AnimationFrameIndex = 0; AnimationFrameIndex < Tile->AnimationFrameCount; ++AnimationFrameIndex)
                    {
                        tile_animation_frame *TileAnimationFrame = Tile->AnimationFrames + AnimationFrameIndex;
                        animation_frame *AnimationFrame = Animation->AnimationFrames + AnimationFrameIndex;

                        AnimationFrame->Duration = TileAnimationFrame->Duration;
                        AnimationFrame->Width01 = (f32)Tileset.TileWidthInPixels / (f32)Tileset.Image.Width;
                        AnimationFrame->Height01 = (f32)Tileset.TileHeightInPixels / (f32)Tileset.Image.Height;

                        s32 TileX = TileAnimationFrame->TileId % Tileset.Columns;
                        s32 TileY = TileAnimationFrame->TileId / Tileset.Columns;

                        AnimationFrame->XOffset01 = 
                            (f32)(TileX * (Tileset.TileWidthInPixels + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.Image.Width;
                        AnimationFrame->YOffset01 = 
                            (f32)(TileY * (Tileset.TileHeightInPixels + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.Image.Height;

                        AnimationFrame->CurrentXOffset01 = AnimationFrame->XOffset01;
                        AnimationFrame->CurrentYOffset01 = AnimationFrame->YOffset01;
                    }
                }
            }
        }


        vec2 ScreenCenterInMeters = vec2(
            GameState->ScreenWidthInMeters / 2.f,
            GameState->ScreenHeightInMeters / 2.f
        );

        mat4 *TileInstanceModels = PushArray<mat4>(&GameState->WorldArena, GameState->TotalTileCount);
        vec2 *TileInstanceUVOffsets01 = PushArray<vec2>(&GameState->WorldArena, GameState->TotalTileCount);
        
        GameState->Boxes = PushArray<aabb>(&GameState->WorldArena, GameState->TotalBoxCount);
        mat4 *BoxInstanceModels = PushArray<mat4>(&GameState->WorldArena, GameState->TotalBoxCount);

        u32 TileInstanceIndex = 0;
        u32 BoxIndex = 0;
        for (u32 TileLayerIndex = 0; TileLayerIndex < GameState->Map.TileLayerCount; ++TileLayerIndex)
        {
            for (u32 ChunkIndex = 0; ChunkIndex < GameState->Map.TileLayers[TileLayerIndex].ChunkCount; ++ChunkIndex)
            {
                for (u32 GIDIndex = 0; GIDIndex < GameState->Map.TileLayers[TileLayerIndex].Chunks[ChunkIndex].GIDCount; ++GIDIndex)
                {
                    map_chunk *Chunk = GameState->Map.TileLayers[TileLayerIndex].Chunks + ChunkIndex;
                    u32 GID = Chunk->GIDs[GIDIndex];
                    if (GID > 0)
                    {
                        u32 TileID = GID - TilesetFirstGID;

                        // TileInstanceModel
                        mat4 *TileInstanceModel = TileInstanceModels + TileInstanceIndex;
                        *TileInstanceModel = mat4(1.f);

                        s32 TileMapX = Chunk->X + (GIDIndex % Chunk->Width);
                        s32 TileMapY = Chunk->Y + (GIDIndex / Chunk->Height);

                        f32 TileXMeters = ScreenCenterInMeters.x + TileMapX * Tileset->TileWidthInMeters;
                        f32 TileYMeters = ScreenCenterInMeters.y - TileMapY * Tileset->TileHeightInMeters;

                        *TileInstanceModel = glm::translate(*TileInstanceModel, vec3(TileXMeters, TileYMeters, 0.f));
                        *TileInstanceModel = glm::scale(*TileInstanceModel, 
                            vec3(Tileset->TileWidthInMeters, Tileset->TileHeightInMeters, 0.f));

                        // TileInstanceUVOffset01
                        vec2 *TileInstanceUVOffset01 = TileInstanceUVOffsets01 + TileInstanceIndex;

                        s32 TileX = TileID % Tileset->Columns;
                        s32 TileY = TileID / Tileset->Columns;

                        *TileInstanceUVOffset01 = vec2(
                            (f32)(TileX * (Tileset->TileWidthInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Width,
                            (f32)(TileY * (Tileset->TileHeightInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Height
                        );

                        tile_meta_info *TileInfo = GetTileMetaInfo(Tileset, TileID);
                        if (TileInfo)
                        {
                            // Box
                            for (u32 CurrentBoxIndex = 0; CurrentBoxIndex < TileInfo->BoxCount; ++CurrentBoxIndex)
                            {
                                aabb *Box = GameState->Boxes + BoxIndex;

                                Box->Position.x = TileXMeters + 
                                    TileInfo->Boxes[CurrentBoxIndex].Position.x * Tileset->TilesetWidthPixelsToMeters;
                                Box->Position.y = TileYMeters + 
                                    ((Tileset->TileHeightInPixels - 
                                        TileInfo->Boxes[CurrentBoxIndex].Position.y - 
                                        TileInfo->Boxes[CurrentBoxIndex].Size.y) * Tileset->TilesetHeightPixelsToMeters);

                                Box->Size.x = TileInfo->Boxes[CurrentBoxIndex].Size.x * Tileset->TilesetWidthPixelsToMeters;
                                Box->Size.y = TileInfo->Boxes[CurrentBoxIndex].Size.y * Tileset->TilesetHeightPixelsToMeters;

                                mat4 *BoxInstanceModel = BoxInstanceModels + BoxIndex;
                                *BoxInstanceModel = mat4(1.f);

                                *BoxInstanceModel = glm::translate(*BoxInstanceModel, 
                                    vec3(Box->Position.x, Box->Position.y, 0.f));
                                *BoxInstanceModel = glm::scale(*BoxInstanceModel,
                                    vec3(Box->Size.x, Box->Size.y, 0.f));

                                ++BoxIndex;
                            }
                        }

                        ++TileInstanceIndex;
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
            for (u32 ObjectIndex = 0; ObjectIndex < GameState->Map.ObjectLayers[ObjectLayerIndex].ObjectCount; ++ObjectIndex)
            {
                map_object *Object = GameState->Map.ObjectLayers[ObjectLayerIndex].Objects + ObjectIndex;
                entity *Entity = Entities + EntityInstanceIndex;

                *Entity = {};
                Entity->ID = Object->ID;
                // tile objects have their position at bottom-left (https://github.com/bjorn/tiled/issues/91)
                Entity->Position = vec2(
                    Object->X * Tileset->TilesetWidthPixelsToMeters, 
                    (Object->Y - Object->Height) * Tileset->TilesetWidthPixelsToMeters
                );
                Entity->Size = vec2(
                    Object->Width * Tileset->TilesetHeightPixelsToMeters, 
                    Object->Height * Tileset->TilesetHeightPixelsToMeters
                );

                Entity->Type = Object->Type;

                entity_render_info *EntityRenderInfo = GameState->EntityRenderInfos + EntityInstanceIndex;
                // todo: very fragile (deal with QuadVerticesSize part)!
                EntityRenderInfo->Offset = EntityInstanceIndex * sizeof(entity_render_info) + QuadVerticesSize;

                BoxModelOffset += BoxIndex * sizeof(mat4);
                EntityRenderInfo->BoxModelOffset = BoxModelOffset;

                if (Object->GID)
                {
                    u32 TileID = Object->GID - TilesetFirstGID;

                    // EntityInstanceModel
                    EntityRenderInfo->InstanceModel = mat4(1.f);

                    f32 EntityWorldXInMeters = ScreenCenterInMeters.x + Entity->Position.x;
                    f32 EntityWorldYInMeters = ScreenCenterInMeters.y - Entity->Position.y;

                    EntityRenderInfo->InstanceModel = glm::translate(EntityRenderInfo->InstanceModel, 
                        vec3(EntityWorldXInMeters, EntityWorldYInMeters, 0.f));
                    EntityRenderInfo->InstanceModel = glm::scale(EntityRenderInfo->InstanceModel,
                        vec3(Entity->Size.x, Entity->Size.y, 0.f));

                    // EntityInstanceUVOffset01
                    s32 TileX = TileID % Tileset->Columns;
                    s32 TileY = TileID / Tileset->Columns;

                    EntityRenderInfo->InstanceUVOffset01 = vec2(
                        (f32)(TileX * (Tileset->TileWidthInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Width,
                        (f32)(TileY * (Tileset->TileHeightInPixels + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Height
                    );

                    tile_meta_info *EntityTileInfo = GetTileMetaInfo(Tileset, TileID);

                    if (EntityTileInfo)
                    {
                        Entity->BoxCount = EntityTileInfo->BoxCount;
                        Entity->Boxes = PushArray<aabb_info>(&GameState->WorldArena, Entity->BoxCount);

                        // Box
                        for (u32 CurrentBoxIndex = 0; CurrentBoxIndex < EntityTileInfo->BoxCount; ++CurrentBoxIndex)
                        {
                            aabb *Box = GameState->Boxes + BoxIndex;

                            Box->Position.x = EntityWorldXInMeters +
                                EntityTileInfo->Boxes[CurrentBoxIndex].Position.x * Tileset->TilesetWidthPixelsToMeters;
                            Box->Position.y = EntityWorldYInMeters +
                                ((Tileset->TileHeightInPixels -
                                    EntityTileInfo->Boxes[CurrentBoxIndex].Position.y -
                                    EntityTileInfo->Boxes[CurrentBoxIndex].Size.y) * Tileset->TilesetHeightPixelsToMeters);

                            Box->Size.x = EntityTileInfo->Boxes[CurrentBoxIndex].Size.x * Tileset->TilesetWidthPixelsToMeters;
                            Box->Size.y = EntityTileInfo->Boxes[CurrentBoxIndex].Size.y * Tileset->TilesetHeightPixelsToMeters;

                            mat4 *BoxInstanceModel = BoxInstanceModels + BoxIndex;
                            *BoxInstanceModel = mat4(1.f);

                            *BoxInstanceModel = glm::translate(*BoxInstanceModel,
                                vec3(Box->Position.x, Box->Position.y, 0.f));
                            *BoxInstanceModel = glm::scale(*BoxInstanceModel,
                                vec3(Box->Size.x, Box->Size.y, 0.f));

                            Entity->Boxes[CurrentBoxIndex].Box = Box;
                            Entity->Boxes[CurrentBoxIndex].Model = BoxInstanceModel;

                            ++BoxIndex;
                        }
                    }

                    Entity->RenderInfo = EntityRenderInfo;

                    // DrawableEntity
                    entity *DrawableEntity = GameState->DrawableEntities + EntityInstanceIndex;
                    // todo: hmm...
                    *DrawableEntity = *Entity;

                    if (DrawableEntity->Type == entity_type::PLAYER)
                    {
                        GameState->Player = DrawableEntity;
                        ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_IDLE);
                    }

                    ++EntityInstanceIndex;
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
            Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + Offset(entity_render_info, InstanceModel));
        }

        {
            vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 2;
            Attribute->Index = 2;
            Attribute->Size = 4;
            Attribute->Type = GL_FLOAT;
            Attribute->Normalized = GL_FALSE;
            Attribute->Stride = sizeof(entity_render_info);
            Attribute->Divisor = 1;
            Attribute->OffsetPointer = (void *)(QuadVerticesSize + Offset(entity_render_info, InstanceModel) + sizeof(vec4));
        }

        {
            vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 3;
            Attribute->Index = 3;
            Attribute->Size = 4;
            Attribute->Type = GL_FLOAT;
            Attribute->Normalized = GL_FALSE;
            Attribute->Stride = sizeof(entity_render_info);
            Attribute->Divisor = 1;
            Attribute->OffsetPointer = (void *)(QuadVerticesSize + Offset(entity_render_info, InstanceModel) + 2 * sizeof(vec4));
        }

        {
            vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 4;
            Attribute->Index = 4;
            Attribute->Size = 4;
            Attribute->Type = GL_FLOAT;
            Attribute->Normalized = GL_FALSE;
            Attribute->Stride = sizeof(entity_render_info);
            Attribute->Divisor = 1;
            Attribute->OffsetPointer = (void *)(QuadVerticesSize + Offset(entity_render_info, InstanceModel) + 3 * sizeof(vec4));
        }

        {
            vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 5;
            Attribute->Index = 5;
            Attribute->Size = 2;
            Attribute->Type = GL_FLOAT;
            Attribute->Normalized = GL_FALSE;
            Attribute->Stride = sizeof(entity_render_info);
            Attribute->Divisor = 1;
            Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + Offset(entity_render_info, InstanceUVOffset01));
        }

        {
            vertex_buffer_attribute *Attribute = GameState->DrawableEntitiesVertexBuffer.AttributesLayout->Attributes + 6;
            Attribute->Index = 6;
            Attribute->Size = 1;
            Attribute->Type = GL_UNSIGNED_INT;
            Attribute->Stride = sizeof(u32);
            Attribute->Divisor = 1;
            Attribute->OffsetPointer = (void *)((u64)QuadVerticesSize + Offset(entity_render_info, Flipped));
        }

        SetupVertexBuffer(Renderer, &GameState->DrawableEntitiesVertexBuffer);
        #pragma endregion


        #pragma region Border

        GameState->BorderVertexBuffer = {};
        GameState->BorderVertexBuffer.Size = QuadVerticesSize;
        GameState->BorderVertexBuffer.Usage = GL_STATIC_DRAW;

        GameState->BorderVertexBuffer.DataLayout = PushStruct<vertex_buffer_data_layout>(&GameState->WorldArena);
        GameState->BorderVertexBuffer.DataLayout->SubBufferCount = 1;
        GameState->BorderVertexBuffer.DataLayout->SubBuffers = PushArray<vertex_sub_buffer>(
            &GameState->WorldArena, GameState->BorderVertexBuffer.DataLayout->SubBufferCount);

        {
            vertex_sub_buffer *SubBuffer = GameState->BorderVertexBuffer.DataLayout->SubBuffers + 0;
            SubBuffer->Offset = 0;
            SubBuffer->Size = QuadVerticesSize;
            SubBuffer->Data = QuadVertices;
        }

        GameState->BorderVertexBuffer.AttributesLayout = PushStruct<vertex_buffer_attributes_layout>(&GameState->WorldArena);
        GameState->BorderVertexBuffer.AttributesLayout->AttributeCount = 1;
        GameState->BorderVertexBuffer.AttributesLayout->Attributes = PushArray<vertex_buffer_attribute>(
            &GameState->WorldArena, GameState->BorderVertexBuffer.AttributesLayout->AttributeCount);

        {
            vertex_buffer_attribute *Attribute = GameState->BorderVertexBuffer.AttributesLayout->Attributes + 0;
            Attribute->Index = 0;
            Attribute->Size = 4;
            Attribute->Type = GL_FLOAT;
            Attribute->Normalized = GL_FALSE;
            Attribute->Stride = sizeof(vec4);
            Attribute->Divisor = 0;
            Attribute->OffsetPointer = (void *)0;
        }

        SetupVertexBuffer(Renderer, &GameState->BorderVertexBuffer);
        #pragma endregion

        GameState->UpdateRate = 0.01f;   // 10 ms
        GameState->Lag = 0.f;

        GameState->Zoom = 1.f / 1.f;
        GameState->Camera = GameState->Player->Position;

        Renderer->glEnable(GL_BLEND);
        Renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Renderer->glEnable(GL_STENCIL_TEST);
        Renderer->glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        Renderer->glClearColor(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, 1.f);

        GameState->IsInitialized = true;
    }

    Renderer->glViewport(0, 0, ScreenWidth, ScreenHeight);

    GameState->Lag += Params->Delta;

    ProcessInput(GameState, &Params->Input, Params->Delta);

    while (GameState->Lag >= GameState->UpdateRate)
    {
        GameState->Lag -= GameState->UpdateRate;

        f32 Dt = 0.1f;

        // friction imitation
        GameState->Player->Acceleration.x += -8.f * GameState->Player->Velocity.x;
        GameState->Player->Acceleration.y += -0.01f * GameState->Player->Velocity.y;

        GameState->Player->Velocity += GameState->Player->Acceleration * Dt;

        vec2 Move = 0.5f * GameState->Player->Acceleration * Dt * Dt + GameState->Player->Velocity * Dt;

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
        GameState->Player->Position.y -= UpdatedMove.y;

        GameState->Player->Acceleration.x = 0.f;
        // gravity
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
                animation *SquashAnimation = GetAnimation(GameState, ANIMATION_PLAYER_SQUASH);
                SquashAnimation->Next = GetAnimation(GameState, ANIMATION_PLAYER_IDLE);

                ChangeAnimation(GameState, GameState->Player, SquashAnimation, false);
            }
        }

        if (GameState->Player->Velocity.y > 0.f)
        {
            // todo: deal with ifs
            if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_UP))
            {
                ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_JUMP_UP);
            }
        }
        else if (GameState->Player->Velocity.y < 0.f)
        {
            if (GameState->Player->CurrentAnimation != GetAnimation(GameState, ANIMATION_PLAYER_JUMP_DOWN))
            {
                ChangeAnimation(GameState, GameState->Player, ANIMATION_PLAYER_JUMP_DOWN);
            }
        }

        // todo: store 1/size as well
        GameState->Player->RenderInfo->InstanceModel = glm::scale(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(1.f / GameState->Player->Size, 1.f)
        );
        GameState->Player->RenderInfo->InstanceModel = glm::translate(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(UpdatedMove, 0.f)
        );
        GameState->Player->RenderInfo->InstanceModel = glm::scale(
            GameState->Player->RenderInfo->InstanceModel, 
            vec3(GameState->Player->Size, 1.f)
        );

        for (u32 PlayerBoxModelIndex = 0; PlayerBoxModelIndex < GameState->Player->BoxCount; ++PlayerBoxModelIndex)
        {
            aabb_info *PlayerBox = GameState->Player->Boxes + PlayerBoxModelIndex;

            PlayerBox->Box->Position += UpdatedMove;

            *PlayerBox->Model = glm::scale(*PlayerBox->Model, vec3(1.f / PlayerBox->Box->Size, 1.f));
            *PlayerBox->Model = glm::translate(*PlayerBox->Model, vec3(UpdatedMove, 0.f));
            *PlayerBox->Model = glm::scale(*PlayerBox->Model, vec3(PlayerBox->Box->Size, 1.f));
        }

        // camera
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

        GameState->Camera.y -= UpdatedMove.y;

        //std::cout << "Player->Position: " << GameState->Player->Position.x << ", " << GameState->Player->Position.y << std::endl;
        //std::cout << "Camera: " << GameState->Camera.x << ", " << GameState->Camera.y << std::endl;

        GameState->Projection = glm::ortho(
            -GameState->ScreenWidthInMeters / 2.f * GameState->Zoom, 
            GameState->ScreenWidthInMeters / 2.f * GameState->Zoom,
           -GameState->ScreenHeightInMeters / 2.f * GameState->Zoom,
            GameState->ScreenHeightInMeters / 2.f * GameState->Zoom
        );

        mat4 View = mat4(1.f);
        View = glm::translate(View, vec3(-GameState->Camera.x, GameState->Camera.y, 0.f));
        View = glm::translate(View, vec3(-GameState->ScreenWidthInMeters / 2.f, -GameState->ScreenHeightInMeters / 2.f, 0.f));

        GameState->VP = GameState->Projection * View;
    }

    Renderer->glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    Renderer->glBindBuffer(GL_UNIFORM_BUFFER, GameState->UBO);
    Renderer->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &GameState->VP);

    Renderer->glStencilFunc(GL_ALWAYS, 1, 0x00);
    Renderer->glStencilMask(0x00);

    // Draw tiles
    Renderer->glUseProgram(GameState->TilesShaderProgram);
    Renderer->glBindVertexArray(GameState->TilesVertexBuffer.VAO);
    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalTileCount);

    // Draw entities (with borders)
    // 1st render pass: draw objects as normal, writing to the stencil buffer
    Renderer->glStencilFunc(GL_ALWAYS, 1, 0xFF);
    Renderer->glStencilMask(0xFF);

    Renderer->glUseProgram(GameState->DrawableEntitiesShaderProgram);
    Renderer->glBindVertexArray(GameState->DrawableEntitiesVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->DrawableEntitiesVertexBuffer.VBO);

    // animations
    for (u32 EntityIndex = 0; EntityIndex < GameState->TotalDrawableObjectCount; ++EntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + EntityIndex;

        if (Entity->CurrentAnimation)
        {
            animation *Animation = Entity->CurrentAnimation;
            animation_frame *CurrentFrame = GetCurrentAnimationFrame(Animation);

            // todo: ms to sec
            f32 AnimationFrameDuration = CurrentFrame->Duration / 1000.f;

            if (Animation->CurrentTime >= AnimationFrameDuration)
            {
                Animation->CurrentFrameIndex++;

                if (Animation->CurrentFrameIndex >= Animation->AnimationFrameCount)
                {
                    if (Entity->CurrentAnimation->Next)
                    {
                        ChangeAnimation(GameState, Entity, Entity->CurrentAnimation->Next);
                    }
                    else
                    {
                        Entity->CurrentAnimation = nullptr;
                    }
                }

                CurrentFrame = GetCurrentAnimationFrame(Animation);

                CurrentFrame->CurrentXOffset01 = CurrentFrame->XOffset01;
                CurrentFrame->CurrentYOffset01 = CurrentFrame->YOffset01;

                Animation->CurrentTime = 0.f;
            }

            Entity->RenderInfo->InstanceUVOffset01.x = CurrentFrame->CurrentXOffset01;
            Entity->RenderInfo->InstanceUVOffset01.y = CurrentFrame->CurrentYOffset01;

            Animation->CurrentTime += Params->Delta;
        }
    }

    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player->RenderInfo->Offset, sizeof(entity_render_info), GameState->Player->RenderInfo);

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalDrawableObjectCount);

    // 2nd render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
    Renderer->glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    Renderer->glStencilMask(0x00);

    Renderer->glUseProgram(GameState->DrawableEntitiesBorderShaderProgram);

    {
        s32 ColorUniformLocation = GetUniformLocation(Memory, GameState->DrawableEntitiesBorderShaderProgram, "u_Color");
        
        vec4 Color = vec4(0.f, 0.f, 1.f, 1.f);
        SetShaderUniform(Memory, ColorUniformLocation, Color);
    }

    f32 scale = 1.1f;

    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->TotalDrawableObjectCount; ++DrawableEntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + DrawableEntityIndex;

        Entity->RenderInfo->InstanceModel = glm::translate(Entity->RenderInfo->InstanceModel, vec3(Entity->Size / 2.f, 1.f));
        Entity->RenderInfo->InstanceModel = glm::scale(Entity->RenderInfo->InstanceModel, vec3(scale, scale, 1.f));
        Entity->RenderInfo->InstanceModel = glm::translate(Entity->RenderInfo->InstanceModel, vec3(-Entity->Size / 2.f, 1.f));

        Renderer->glBufferSubData(
            GL_ARRAY_BUFFER, Entity->RenderInfo->Offset + Offset(entity_render_info, InstanceModel), 
            sizeof(mat4), &Entity->RenderInfo->InstanceModel
        );
    }

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TotalDrawableObjectCount);

    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->TotalDrawableObjectCount; ++DrawableEntityIndex)
    {
        entity *Entity = GameState->DrawableEntities + DrawableEntityIndex;

        Entity->RenderInfo->InstanceModel = glm::translate(Entity->RenderInfo->InstanceModel, vec3(Entity->Size / 2.f, 1.f));
        Entity->RenderInfo->InstanceModel = glm::scale(Entity->RenderInfo->InstanceModel, vec3(1.f / scale, 1.f / scale, 1.f));
        Entity->RenderInfo->InstanceModel = glm::translate(Entity->RenderInfo->InstanceModel, vec3(-Entity->Size / 2.f, 1.f));

        Renderer->glBufferSubData(
            GL_ARRAY_BUFFER, Entity->RenderInfo->Offset + Offset(entity_render_info, InstanceModel),
            sizeof(mat4), &Entity->RenderInfo->InstanceModel
        );
    }

    Renderer->glStencilFunc(GL_ALWAYS, 1, 0xFF);
    Renderer->glStencilMask(0xFF);

    // Draw collidable regions
    Renderer->glUseProgram(GameState->BoxesShaderProgram);
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

    Renderer->glUseProgram(GameState->BorderShaderProgram);
    Renderer->glBindVertexArray(GameState->BorderVertexBuffer.VAO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->BorderVertexBuffer.VBO);

    {
        // todo: move out
        s32 ModelUniformLocation = GetUniformLocation(Memory, GameState->BorderShaderProgram, "u_Model");
        s32 ColorUniformLocation = GetUniformLocation(Memory, GameState->BorderShaderProgram, "u_Color");

        mat4 Model = mat4(1.f);

        Model = glm::translate(Model, vec3(8.f, 4.f, 0.f));
        Model = glm::scale(Model, vec3(16.f, 6.f, 0.f));

        SetShaderUniform(Memory, ModelUniformLocation, Model);

        vec4 Color = vec4(0.f, 1.f, 0.f, 1.f);
        SetShaderUniform(Memory, ColorUniformLocation, Color);
    }

    Renderer->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //while (GameState->Lag >= GameState->UpdateRate) {
    //    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);

    //    f32 dt = 0.15f;

    //    Bob->Acceleration.x = 0.f;

    //    // friction imitation
    //    // todo: take scale into account!
    //    Bob->Acceleration.x += -0.5f * Bob->Velocity.x;
    //    Bob->Velocity.x += Bob->Acceleration.x * dt;

    //    Bob->Acceleration.y += -0.01f * Bob->Velocity.y;
    //    Bob->Velocity.y += Bob->Acceleration.y * dt;

    //    vec2 move = 0.5f * Bob->Acceleration * dt * dt + Bob->Velocity * dt;

    //    vec2 oldPosition = Bob->Position;
    //    vec2 time = vec2(1.f);

    //    for (u32 EntityIndex = 0; EntityIndex < GameState->EntitiesCount; ++EntityIndex) {
    //        vec2 t = sweptAABB(oldPosition, move, GameState->Entities[EntityIndex].Box, Bob->Box.Size);

    //        if (t.x >= 0.f && t.x < time.x) time.x = t.x;
    //        if (t.y >= 0.f && t.y < time.y) time.y = t.y;
    //    }

    //    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
    //        drawable_entity *Entity = &GameState->DrawableEntities[DrawableEntityIndex];

    //        if (Entity->Type == tile_type::REFLECTOR || Entity->Type == tile_type::PLATFORM) {
    //            if (!Entity->Collides) break;

    //            vec2 t = sweptAABB(oldPosition, move, Entity->Box, Bob->Box.Size);

    //            if (t.x >= 0.f && t.x < time.x) time.x = t.x;
    //            if (t.y >= 0.f && t.y < time.y) time.y = t.y;

    //            if (Entity->Type == tile_type::REFLECTOR) {
    //                b32 swooshCollide = intersectAABB(Swoosh->Box, Entity->Box);
    //                if (!Entity->UnderEffect && swooshCollide) {
    //                    Entity->UnderEffect = true;
    //                    Entity->IsRotating = true;
    //                }

    //                if (Entity->IsRotating) {
    //                    Entity->Rotation += 5.f;
    //                    Renderer->glBufferSubData(GL_ARRAY_BUFFER, Entity->Offset + Offset(drawable_entity, Rotation), sizeof(u32), &Entity->Rotation);

    //                    if (0.f < Entity->Rotation && Entity->Rotation <= 90.f) {
    //                        if (Entity->Rotation == 90.f) {
    //                            Entity->IsRotating = false;
    //                            break;
    //                        }
    //                    }
    //                    if (90.f < Entity->Rotation && Entity->Rotation <= 180.f) {
    //                        if (Entity->Rotation == 180.f) {
    //                            Entity->IsRotating = false;
    //                            break;
    //                        }
    //                    }
    //                    if (180.f < Entity->Rotation && Entity->Rotation <= 270.f) {
    //                        if (Entity->Rotation == 270.f) {
    //                            Entity->IsRotating = false;
    //                            break;
    //                        }
    //                    }
    //                    if (270.f < Entity->Rotation && Entity->Rotation <= 360.f) {
    //                        if (Entity->Rotation == 360.f) {
    //                            Entity->IsRotating = false;
    //                            Entity->Rotation = 0.f;
    //                            break;
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //    }

    //    if (time.x < 1.f) {
    //        Bob->Velocity.x = 0.f;
    //    }
    //    if (time.y < 1.f) {
    //        Bob->Velocity.y = 0.f;

    //        if (time.y > 0.f && move.y > 0.f && Bob->CurrentAnimation != Bob->Animations[1]) {
    //            Bob->CurrentAnimation = Bob->Animations[1];
    //            Bob->XAnimationOffset = 0.f;
    //        }
    //    }
    //    if (time.y == 1.f) {
    //        if (Bob->Velocity.y > 0.f) {
    //            if (Bob->CurrentAnimation != Bob->Animations[3]) {
    //                Bob->CurrentAnimation = Bob->Animations[5];
    //                Bob->XAnimationOffset = 0.f;
    //            }
    //        }
    //        else {
    //            if (Bob->CurrentAnimation != Bob->Animations[3]) {
    //                Bob->CurrentAnimation = Bob->Animations[4];
    //                Bob->XAnimationOffset = 0.f;
    //            }
    //        }
    //    }

    //    vec2 updatedMove = move * time;

    //    Bob->Position.x = oldPosition.x + updatedMove.x;
    //    Bob->Position.y = oldPosition.y + updatedMove.y;

    //    //Bob->Position.x = clamp(Bob->Position.x, 0.f, (f32)TILE_SIZE.x * GameState->Map.Width - TILE_SIZE.x);
    //    //Bob->Position.y = clamp(Bob->Position.y, 0.f, (f32)TILE_SIZE.y * GameState->Map.Height - TILE_SIZE.y);

    //    Bob->Box.Position = Bob->Position;

    //    Bob->Acceleration.y = 10.f;

    //    vec2 idleArea = { 2 * TILE_SIZE.x, 1 * TILE_SIZE.y };

    //    if (updatedMove.x > 0.f) {
    //        if (Bob->Position.x + TILE_SIZE.x > GameState->Camera.x + ScreenWidth / 2 + idleArea.x) {
    //            GameState->Camera.x += updatedMove.x;
    //        }
    //    }
    //    else if (updatedMove.x < 0.f) {
    //        if (Bob->Position.x < GameState->Camera.x + ScreenWidth / 2 - idleArea.x) {
    //            GameState->Camera.x += updatedMove.x;
    //        }
    //    }

    //    if (updatedMove.y > 0.f) {
    //        if (Bob->Position.y + TILE_SIZE.y > GameState->Camera.y + ScreenHeight / 2 + idleArea.y) {
    //            GameState->Camera.y += updatedMove.y;
    //        }
    //    }
    //    else if (updatedMove.y < 0.f) {
    //        if (Bob->Position.y < GameState->Camera.y + ScreenHeight / 2 - idleArea.y) {
    //            GameState->Camera.y += updatedMove.y;
    //        }
    //    }

    //    GameState->Camera.x = GameState->Camera.x > 0.f ? GameState->Camera.x : 0.f;
    //    GameState->Camera.y = GameState->Camera.y > 0.f ? GameState->Camera.y : 0.f;

    //    //if (TILE_SIZE.x * GameState->Map.Width - ScreenWidth >= 0) {
    //    //    GameState->Camera.x = clamp(GameState->Camera.x, 0.f, (f32)TILE_SIZE.x * GameState->Map.Width - ScreenWidth);
    //    //}
    //    //if (TILE_SIZE.y * GameState->Map.Height - ScreenHeight >= 0) {
    //    //    GameState->Camera.y = clamp(GameState->Camera.y, 0.f, (f32)TILE_SIZE.y * GameState->Map.Height - ScreenHeight);
    //    //}

    //    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);

    //    for (u32 i = 0; i < GameState->ParticleEmittersIndex; ++i) {
    //        particle_emitter *Charge = &GameState->ParticleEmitters[i];

    //        vec2 oldChargePosition = Charge->Box.Position;
    //        vec2 chargeMove = Charge->Velocity * dt;
    //        vec2 chargeTime = vec2(1.f);

    //        for (u32 j = 0; j < GameState->DrawableEntitiesCount; ++j) {
    //            drawable_entity *Entity = &GameState->DrawableEntities[j];

    //            if (Entity->Type == tile_type::REFLECTOR) {
    //                aabb reflectorBox = Entity->Box;

    //                vec2 t = sweptAABB(oldChargePosition, chargeMove, reflectorBox, Charge->Box.Size);

    //                // if not colliding
    //                if (!((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f))) {
    //                    if (!intersectAABB(Charge->Box, reflectorBox)) {
    //                        if (Charge->ReflectorIndex == (s32)j) {
    //                            Charge->StopProcessingCollision = false;
    //                            Charge->ReflectorIndex = -1;
    //                        }
    //                        continue;
    //                    }
    //                }

    //                // if collides check direction of the charge
    //                // check reflector's angle
    //                // dimiss charge if it's coming from the wrong side
    //                // proceed with new collision rule otherwise.

    //                if (Charge->StopProcessingCollision && Charge->ReflectorIndex == (s32)j) continue;

    //                aabb testBox = {};

    //                if (chargeMove.x > 0.f) {
    //                    if (Entity->Rotation == 180.f || Entity->Rotation == 270.f) {
    //                        testBox.Position.x = reflectorBox.Position.x + reflectorBox.Size.x / 2.f + Charge->Box.Size.x / 2.f;
    //                        testBox.Position.y = reflectorBox.Position.y;
    //                        testBox.Size.x = reflectorBox.Size.x / 2.f - Charge->Box.Size.x / 2.f;
    //                        testBox.Size.y = reflectorBox.Size.y;

    //                        vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.Size);

    //                        if (0.f <= t.x && t.x < 1.f) {
    //                            chargeTime.x = t.x;

    //                            Charge->Velocity.x = 0.f;
    //                            Charge->Velocity.y = Entity->Rotation == 180.f ? chargeVelocity : -chargeVelocity;
    //                            Charge->StopProcessingCollision = true;
    //                            Charge->ReflectorIndex = (s32)j;
    //                        }
    //                    }
    //                    else {
    //                        // collided with outer border: stop processing
    //                        chargeTime = t;
    //                        Charge->IsFading = true;
    //                    }
    //                }
    //                else if (chargeMove.x < 0.f) {
    //                    if (Entity->Rotation == 0.f || Entity->Rotation == 90.f) {
    //                        testBox.Position.x = reflectorBox.Position.x;
    //                        testBox.Position.y = reflectorBox.Position.y;
    //                        testBox.Size.x = reflectorBox.Size.x / 2.f - Charge->Box.Size.x / 2.f;
    //                        testBox.Size.y = reflectorBox.Size.y;

    //                        vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.Size);

    //                        if (0.f <= t.x && t.x < 1.f) {
    //                            chargeTime.x = t.x;

    //                            Charge->Velocity.x = 0.f;
    //                            Charge->Velocity.y = Entity->Rotation == 0.f ? -chargeVelocity : chargeVelocity;
    //                            Charge->StopProcessingCollision = true;
    //                            Charge->ReflectorIndex = (s32)j;
    //                        }
    //                    }
    //                    else {
    //                        chargeTime = t;
    //                        Charge->IsFading = true;
    //                    }
    //                }
    //                else if (chargeMove.y > 0.f) {
    //                    if (Entity->Rotation == 0.f || Entity->Rotation == 270.f) {
    //                        testBox.Position.x = reflectorBox.Position.x;
    //                        testBox.Position.y = reflectorBox.Position.y + reflectorBox.Size.y / 2.f + Charge->Box.Size.y / 2.f;
    //                        testBox.Size.x = reflectorBox.Size.x;
    //                        testBox.Size.y = reflectorBox.Size.y / 2.f - Charge->Box.Size.y / 2.f;

    //                        vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.Size);

    //                        if (0.f <= t.y && t.y < 1.f) {
    //                            chargeTime.y = t.y;

    //                            Charge->Velocity.x = Entity->Rotation == 0.f ? chargeVelocity : -chargeVelocity;
    //                            Charge->Velocity.y = 0.f;
    //                            Charge->StopProcessingCollision = true;
    //                            Charge->ReflectorIndex = (s32)j;
    //                        }
    //                    }
    //                    else {
    //                        chargeTime = t;
    //                        Charge->IsFading = true;
    //                    }
    //                }
    //                else if (chargeMove.y < 0.f) {
    //                    if (Entity->Rotation == 90.f || Entity->Rotation == 180.f) {
    //                        testBox.Position.x = reflectorBox.Position.x;
    //                        testBox.Position.y = reflectorBox.Position.y;
    //                        testBox.Size.x = reflectorBox.Size.x;
    //                        testBox.Size.y = reflectorBox.Size.y / 2.f - Charge->Box.Size.y / 2.f;

    //                        vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.Size);

    //                        if (0.f <= t.y && t.y < 1.f) {
    //                            chargeTime.y = t.y;

    //                            Charge->Velocity.x = Entity->Rotation == 90.f ? chargeVelocity : -chargeVelocity;
    //                            Charge->Velocity.y = 0.f;
    //                            Charge->StopProcessingCollision = true;
    //                            Charge->ReflectorIndex = (s32)j;
    //                        }
    //                    }
    //                    else {
    //                        chargeTime = t;
    //                        Charge->IsFading = true;
    //                    }
    //                }
    //            }

    //            if (Entity->Id == 52) {
    //                vec2 t = sweptAABB(oldChargePosition, chargeMove, Entity->Box, Charge->Box.Size);

    //                if ((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f)) {
    //                    Entity->CurrentAnimation = &GameState->Lamp.Animations[0];
    //                    chargeTime = t;
    //                    Charge->IsFading = true;
    //                    Charge->TimeLeft = 0.f;

    //                    drawable_entity *platform1 = GetEntityById(GameState, 57);
    //                    drawable_entity *platform2 = GetEntityById(GameState, 60);
    //                    drawable_entity *platform3 = GetEntityById(GameState, 61);

    //                    // todo: i need smth like setTimeout here
    //                    platform1->CurrentAnimation = &GameState->Platform.Animations[0];
    //                    platform1->StartAnimationDelay = 0.1f;
    //                    platform2->CurrentAnimation = &GameState->Platform.Animations[0];
    //                    platform2->StartAnimationDelay = 0.2f;
    //                    platform3->CurrentAnimation = &GameState->Platform.Animations[0];
    //                    platform3->StartAnimationDelay = 0.3f;
    //                }
    //            }
    //        }

    //        chargeMove *= chargeTime;
    //        Charge->Box.Position += chargeMove;
    //        Charge->Position += chargeMove;

    //        //if (Charge->Box.Position.x <= 0.f || Charge->Box.Position.x >= (f32)TILE_SIZE.x * GameState->Map.Width) {
    //        //    Charge->Velocity.x = -Charge->Velocity.x;
    //        //}
    //        //if (Charge->Box.Position.y <= 0.f || Charge->Box.Position.y >= (f32)TILE_SIZE.y * GameState->Map.Height) {
    //        //    Charge->Velocity.y = -Charge->Velocity.y;
    //        //}

    //        b32 chargeCollide = intersectAABB(Swoosh->Box, GameState->ParticleEmitters[0].Box);
    //        if (chargeCollide && Swoosh->ShouldRender && GameState->ChargeSpawnCooldown > 1.f) {
    //            GameState->ChargeSpawnCooldown = 0.f;

    //            particle_emitter NewCharge = GameState->ParticleEmitters[0];        // copy
    //            NewCharge.Particles = PushArray<particle>(&GameState->WorldArena, NewCharge.ParticlesCount);
    //            NewCharge.Velocity.x = Bob->Flipped ? -chargeVelocity : chargeVelocity;

    //            GameState->ParticleEmitters[GameState->ParticleEmittersIndex++] = NewCharge;
    //        }
    //    }

    //    for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
    //        particle_emitter *ParticleEmitter = &GameState->ParticleEmitters[ParticleEmitterIndex];

    //        if (ParticleEmitter->IsFading) {
    //            ParticleEmitter->TimeLeft -= Params->Delta;
    //        }

    //        if (ParticleEmitter->TimeLeft <= 0.f) {
    //            // todo: read access violation
    //            //particle_emitters->erase(particle_emitters->begin() + i);
    //            --GameState->ParticleEmittersIndex;
    //        }
    //    }

    //    u64 particlesSize = 0;
    //    // todo: use transform feedback instead?
    //    for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
    //        particle_emitter *ParticleEmitter = &GameState->ParticleEmitters[ParticleEmitterIndex];

    //        if (ParticleEmitter->TimeLeft <= 0.f) {
    //            particlesSize += ParticleEmitter->ParticlesCount * sizeof(particle);
    //            continue;
    //        };

    //        for (u32 j = 0; j < ParticleEmitter->NewParticlesCount; ++j) {
    //            u32 unusedParticleIndex = FindFirstUnusedParticle(ParticleEmitter);
    //            ParticleEmitter->LastUsedParticle = unusedParticleIndex;

    //            particle *Particle = &ParticleEmitter->Particles[unusedParticleIndex];

    //            // respawing particle
    //            f32 randomX = randomInRange(-1.f * Scale.x, 1.f * Scale.x);
    //            f32 randomY = randomInRange(-1.f * Scale.y, 1.f * Scale.y);

    //            Particle->Lifespan = 1.f;
    //            Particle->Position.x = ParticleEmitter->Position.x + randomX;
    //            Particle->Position.y = ParticleEmitter->Position.y + randomY;
    //            Particle->Size = { 0.2f * TILE_SIZE.x, 0.2f * TILE_SIZE.y };
    //            Particle->Velocity = { 0.f, 0.f };
    //            Particle->Acceleration = { randomX * 10.f, 10.f };
    //            Particle->UV = vec2((13 * (Tileset->TileSize.y + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Height,
    //                (16 * (Tileset->TileSize.y + Tileset->Spacing) + Tileset->Margin) / (f32)Tileset->Image.Height);
    //            Particle->Alpha = 1.f;
    //        }

    //        for (u32 j = 0; j < ParticleEmitter->ParticlesCount; ++j) {
    //            particle *P = &ParticleEmitter->Particles[j];
    //            f32 dt = ParticleEmitter->Dt;

    //            if (P->Lifespan > 0.f) {
    //                P->Lifespan -= (f32)dt;
    //                P->Velocity = P->Acceleration * dt;
    //                P->Position.x += randomInRange(-1.f, 1.f);
    //                P->Position.y += randomInRange(-1.f, 1.f);
    //                P->Alpha -= (f32)dt * 1.f;
    //                P->Size -= (f32)dt * 1.f;
    //            }
    //        }

    //        Renderer->glBufferSubData(GL_ARRAY_BUFFER, particlesSize, ParticleEmitter->ParticlesCount * sizeof(particle),
    //            ParticleEmitter->Particles);

    //        particlesSize += ParticleEmitter->ParticlesCount * sizeof(particle);
    //    }

    //    GameState->Lag -= GameState->UpdateRate;
    //}

    //GameState->ChargeSpawnCooldown += Params->Delta;

    //mat4 view = mat4(1.0f);
    //view = glm::translate(view, vec3(-GameState->Camera, 0.f));
    //setShaderUniform(Memory, GameState->ViewUniformLocation, view);

    //--- drawing tilemap ---
    //Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOTiles);
    //setShaderUniform(Memory, GameState->TypeUniformLocation, 1);

    //mat4 model = mat4(1.0f);
    //model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    //setShaderUniform(Memory, GameState->ModelUniformLocation, model);

    //Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TilesCount);

    //--- bob ---
    //f32 bobXOffset = (f32)(Bob->CurrentAnimation.X * (Tileset->TileSize.x + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Width;
    //f32 bobYOffset = (f32)(Bob->CurrentAnimation.Y * (Tileset->TileSize.y + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Height;

    //if (Bob->FrameTime >= Bob->CurrentAnimation.Delay) {
    //    Bob->XAnimationOffset += (GameState->SpriteWidth + (f32)Tileset->Spacing / Tileset->Image.Width) * Bob->CurrentAnimation.Size;
    //    if (Bob->XAnimationOffset >= ((Bob->CurrentAnimation.Frames * Tileset->TileSize.x * Bob->CurrentAnimation.Size) / (f32)Tileset->Image.Width)) {
    //        Bob->XAnimationOffset = 0.f;
    //        if (Bob->CurrentAnimation == Bob->Animations[1] || Bob->CurrentAnimation == Bob->Animations[3]) {
    //            Bob->CurrentAnimation = Bob->Animations[0];
    //        }
    //    }

    //    Bob->FrameTime = 0.0f;
    //}
    //Bob->FrameTime += Params->Delta;

    //model = mat4(1.0f);
    //model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    //setShaderUniform(Memory, GameState->ModelUniformLocation, model);

    //f32 effectXOffset = (f32)(Swoosh->CurrentAnimation.X * (Tileset->TileSize.x + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Width;
    //f32 effectYOffset = (f32)(Swoosh->CurrentAnimation.Y * (Tileset->TileSize.y + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Height;

    //if (Swoosh->FrameTime >= Swoosh->CurrentAnimation.Delay) {
    //    Swoosh->XAnimationOffset += (GameState->SpriteWidth + (f32)Tileset->Spacing / Tileset->Image.Width) * Swoosh->CurrentAnimation.Size;
    //    if (Swoosh->XAnimationOffset >= ((Swoosh->CurrentAnimation.Frames * Tileset->TileSize.x * Swoosh->CurrentAnimation.Size) / (f32)Tileset->Image.Width)) {
    //        Swoosh->XAnimationOffset = 0.f;
    //        Swoosh->ShouldRender = false;
    //    }

    //    Swoosh->FrameTime = 0.0f;
    //}

    //if (Swoosh->ShouldRender) {
    //    Swoosh->FrameTime += Params->Delta;
    //}

    //Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);
    //// handling animations on all entities
    //for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
    //    drawable_entity *Entity = &GameState->DrawableEntities[DrawableEntityIndex];

    //    if (Entity->CurrentAnimation) {
    //        if (Entity->StartAnimationDelayTimer < Entity->StartAnimationDelay) {
    //            Entity->StartAnimationDelayTimer += Params->Delta;
    //            break;
    //        }

    //        f32 entityXOffset = (f32)(Entity->CurrentAnimation->X * (Tileset->TileSize.x + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Width;
    //        f32 entityYOffset = (f32)(Entity->CurrentAnimation->Y * (Tileset->TileSize.y + Tileset->Spacing) + Tileset->Margin) / Tileset->Image.Height;

    //        if (Entity->FrameTime >= Entity->CurrentAnimation->Delay) {
    //            if (Entity->CurrentAnimation->Direction == direction::RIGHT) {
    //                Entity->XAnimationOffset += (GameState->SpriteWidth + (f32)Tileset->Spacing / Tileset->Image.Width) * Entity->CurrentAnimation->Size;
    //            }
    //            else if (Entity->CurrentAnimation->Direction == direction::LEFT) {
    //                Entity->XAnimationOffset -= (GameState->SpriteWidth + (f32)Tileset->Spacing / Tileset->Image.Width) * Entity->CurrentAnimation->Size;
    //            }

    //            Entity->UV = vec2(entityXOffset + Entity->XAnimationOffset, entityYOffset);
    //            Renderer->glBufferSubData(GL_ARRAY_BUFFER, Entity->Offset + Offset(drawable_entity, UV), sizeof(vec2), &Entity->UV);

    //            if (abs(Entity->XAnimationOffset) >= ((Entity->CurrentAnimation->Frames * Tileset->TileSize.x * Entity->CurrentAnimation->Size) / (f32)Tileset->Image.Width)) {
    //                Entity->XAnimationOffset = 0.f;

    //                Entity->StartAnimationDelayTimer = 0.f;
    //                Entity->StartAnimationDelay = 0.f;
    //                Entity->CurrentAnimation = nullptr;
    //            }

    //            Entity->FrameTime = 0.0f;
    //        }
    //        Entity->FrameTime += Params->Delta;
    //    }
    //}

    ////--- drawing entities ---
    //setShaderUniform(Memory, GameState->TypeUniformLocation, 3);

    //GameState->Player.UV = vec2(bobXOffset + Bob->XAnimationOffset, bobYOffset);
    //GameState->Player.Position = Bob->Position;
    //GameState->Player.Box.Position = Bob->Box.Position;
    //GameState->Player.Flipped = Bob->Flipped;

    //GameState->SwooshEffect.UV = vec2(effectXOffset + Swoosh->XAnimationOffset, effectYOffset);
    //GameState->SwooshEffect.Position = Swoosh->Position;
    //GameState->SwooshEffect.Box.Position = Swoosh->Box.Position;
    //GameState->SwooshEffect.Flipped = Bob->Flipped;
    //GameState->SwooshEffect.ShouldRender = Swoosh->ShouldRender ? 1 : 0;

    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.Offset + Offset(drawable_entity, Position), 2 * sizeof(f32), &GameState->Player.Position);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.Offset + Offset(drawable_entity, Position), 2 * sizeof(f32), &GameState->SwooshEffect.Position);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.Offset + Offset(drawable_entity, UV), 2 * sizeof(f32), &GameState->Player.UV);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.Offset + Offset(drawable_entity, UV), 2 * sizeof(f32), &GameState->SwooshEffect.UV);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.Offset + Offset(drawable_entity, Flipped), sizeof(u32), &GameState->Player.Flipped);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.Offset + Offset(drawable_entity, Flipped), sizeof(u32), &GameState->SwooshEffect.Flipped);
    //Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.Offset + Offset(drawable_entity, ShouldRender), sizeof(u32), &GameState->SwooshEffect.ShouldRender);

    //Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (s32)GameState->DrawableEntitiesCount);

    ////--- drawing particles ---
    //Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);
    //setShaderUniform(Memory, GameState->TypeUniformLocation, 4);

    //Renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    //// todo: offsets when delete in the middle?
    //s32 totalParticlesCount = 0;
    //for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
    //    totalParticlesCount += GameState->ParticleEmitters[ParticleEmitterIndex].ParticlesCount;
    //}

    //// todo: draw only the ones which lifespan is greater than zero
    //Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, totalParticlesCount);

    //std::cout << delta * 1000.f << " ms" << std::endl;
}