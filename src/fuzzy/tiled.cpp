#include "rapidjson/document.h"
#include "fuzzy.h"

using namespace rapidjson;

internal_function tile_type
GetTileTypeFromString(const char *String)
{
    tile_type Result;

    if (StringEquals(String, "player")) 
    {
        Result = tile_type::PLAYER;
    }
    else if (StringEquals(String, "effect")) 
    {
        Result = tile_type::EFFECT;
    }
    else if (StringEquals(String, "reflector")) 
    {
        Result = tile_type::REFLECTOR;
    }
    else if (StringEquals(String, "lamp")) 
    {
        Result = tile_type::LAMP;
    }
    else if (StringEquals(String, "platform")) 
    {
        Result = tile_type::PLATFORM;
    }
    else 
    {
        Result = tile_type::UNKNOWN;
    }

    return Result;
}

typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>> DocumentType;

internal_function DocumentType
ParseJSON(const char *Json, u64 ValueBufferSize, u64 ParseBufferSize, memory_arena *Region) 
{
    // todo: temporary memory for document storage
    void *ValueBuffer = PushSize(Region, ValueBufferSize);
    void *ParseBuffer = PushSize(Region, ParseBufferSize);

    MemoryPoolAllocator<> ValueAllocator(ValueBuffer, ValueBufferSize);
    MemoryPoolAllocator<> ParseAllocator(ParseBuffer, ParseBufferSize);
    DocumentType Document(&ValueAllocator, ParseBufferSize, &ParseAllocator);
    // todo: error handling
    Document.Parse(Json);

    return Document;
}

internal_function void
LoadTileset(tileset *Tileset, const char *Json, memory_arena *Arena, platform_api *Platform) 
{
    // todo: think more about the sizes
    u64 ValueBufferSize = Kilobytes(512);
    u64 ParseBufferSize = Kilobytes(32);
    DocumentType Document = ParseJSON(Json, ValueBufferSize, ParseBufferSize, Arena);

    const char *ImagePath = Document["image"].GetString();

    char FullImagePath[256];
    snprintf(FullImagePath, sizeof(FullImagePath), "%s%s", "tilesets/", ImagePath);

    Tileset->Image.Memory = Platform->ReadImageFile(FullImagePath, 
        &Tileset->Image.Width, &Tileset->Image.Height, &Tileset->Image.Channels, 0);
    if (!Tileset->Image.Memory) 
    {
        Platform->PrintOutput("Image loading failed:\n");
    }
    assert(Tileset->Image.Memory);

    Tileset->TileWidthInPixels = Document["tilewidth"].GetInt();
    Tileset->TileHeightInPixels = Document["tileheight"].GetInt();
    Tileset->Columns = Document["columns"].GetUint();
    Tileset->Margin = Document["margin"].GetUint();
    Tileset->Spacing = Document["spacing"].GetUint();

    assert(Document.HasMember("properties"));

    const Value& CustomProperties = Document["properties"];
    assert(CustomProperties.IsArray());
    
    for (SizeType CustomPropertyIndex = 0; CustomPropertyIndex < CustomProperties.Size(); ++CustomPropertyIndex) 
    {
        const Value& CustomProperty = CustomProperties[CustomPropertyIndex];
        const char* PropertyName = CustomProperty["name"].GetString();

        if (StringEquals(PropertyName, "TileWidthInMeters"))
        {
            Tileset->TileWidthInMeters = CustomProperty["value"].GetFloat();
        }
        else if (StringEquals(PropertyName, "TileHeightInMeters"))
        {
            Tileset->TileHeightInMeters = CustomProperty["value"].GetFloat();
        }
        else 
        {
            InvalidCodePath;
        }
    }

    Tileset->TileCount = Document["tilecount"].GetUint();
    Tileset->Tiles = PushArray<tile_meta_info>(Arena, Tileset->TileCount);

    if (Document.HasMember("tiles")) 
    {
        const Value& Tiles = Document["tiles"];
        assert(Tiles.IsArray());

        for (SizeType TileIndex = 0; TileIndex < Tiles.Size(); ++TileIndex) 
        {
            const Value& Tile = Tiles[TileIndex];

            u32 TileId = Tile["id"].GetUint();

            tile_meta_info *TileInfo = CreateTileMetaInfo(Tileset, TileId, Arena);
            TileInfo->BoxCount = 0;
            TileInfo->AnimationFrameCount = 0;

            if (Tile.HasMember("type")) 
            {
                TileInfo->Type = GetTileTypeFromString(Tile["type"].GetString());
            }

            if (Tile.HasMember("objectgroup")) 
            {
                const Value& TileObjects = Tile["objectgroup"]["objects"];

                assert(TileObjects.IsArray());

                TileInfo->BoxCount = TileObjects.Size();
                TileInfo->Boxes = PushArray<aabb>(Arena, TileInfo->BoxCount);

                for (SizeType ObjectIndex = 0; ObjectIndex < TileObjects.Size(); ++ObjectIndex) 
                {
                    const Value& Object = TileObjects[ObjectIndex];

                    aabb *Box = TileInfo->Boxes + ObjectIndex;
                    Box->Position = {
                        Object["x"].GetFloat(), Object["y"].GetFloat()
                    };
                    Box->Size = {
                        Object["width"].GetFloat(), Object["height"].GetFloat()
                    };
                }
            }

            if (Tile.HasMember("animation")) 
            {
                const Value& Animation = Tile["animation"];

                assert(Animation.IsArray());

                TileInfo->AnimationFrameCount = Animation.Size();
                TileInfo->AnimationFrames = PushArray<animation_frame>(Arena, TileInfo->AnimationFrameCount);

                for (SizeType AnimationFrameIndex = 0; AnimationFrameIndex < Animation.Size(); ++AnimationFrameIndex) 
                {
                    const Value& Frame = Animation[AnimationFrameIndex];

                    animation_frame *AnimationFrame = TileInfo->AnimationFrames + AnimationFrameIndex;
                    AnimationFrame->Duration = Frame["duration"].GetUint();
                    AnimationFrame->TileId = Frame["tileid"].GetUint();
                }
            }
        }
    }
}

global_variable const char *TILE_LAYER = "tilelayer";
global_variable const char *OBJECT_LAYER = "objectgroup";

void
LoadMap(tiled_map *Map, const char *Json, memory_arena *Arena, platform_api *Platform) 
{
    // todo: think more about the sizes
    u64 ValueBufferSize = Megabytes(32);
    u64 ParseBufferSize = Megabytes(1);
    DocumentType Document = ParseJSON(Json, ValueBufferSize, ParseBufferSize, Arena);

    assert(Document.HasMember("tilesets"));

    // todo: add support for multiple tilesets
    const Value& Tileset = Document["tilesets"].GetArray()[0];

    Map->TilesetCount = 1;
    Map->Tilesets = PushArray<tileset_source>(Arena, Map->TilesetCount);
    Map->Tilesets[0].FirstGID = Tileset["firstgid"].GetUint();

    char *TilesetSource = const_cast<char*>(Tileset["source"].GetString());
    char *TilesetPath = GetLastAfterDelimiter(TilesetSource, '/');

    char FullTilesetPath[256];
    snprintf(FullTilesetPath, sizeof(FullTilesetPath), "%s%s", "tilesets/", TilesetPath);

    char *TilesetJson = (char*)Platform->ReadFile(FullTilesetPath).Contents;

    Map->Tilesets[0].Source = {};
    LoadTileset(&Map->Tilesets[0].Source, TilesetJson, Arena, Platform);

    const Value& Layers = Document["layers"];
    assert(Layers.IsArray());

    u32 TileLayerCount = 0;
    u32 ObjectGroupCount = 0;

    for (SizeType LayerIndex = 0; LayerIndex < Layers.Size(); ++LayerIndex) 
    {
        const Value& Layer = Layers[LayerIndex];
        const char *LayerType = Layer["type"].GetString();

        if (StringEquals(LayerType, TILE_LAYER)) 
        {
            ++TileLayerCount;
        }
        else if (StringEquals(LayerType, OBJECT_LAYER)) 
        {
            ++ObjectGroupCount;
        }
        else 
        {
            InvalidCodePath;
        }
    }

    Map->TileLayerCount = TileLayerCount;
    Map->TileLayers = PushArray<tile_layer>(Arena, Map->TileLayerCount);

    Map->ObjectLayerCount = ObjectGroupCount;
    Map->ObjectLayers = PushArray<object_layer>(Arena, Map->ObjectLayerCount);

    u32 TileLayerIndex = 0;
    u32 ObjectLayerIndex = 0;

    for (SizeType LayerIndex = 0; LayerIndex < Layers.Size(); ++LayerIndex)
    {
        const Value& Layer = Layers[LayerIndex];
        const char *LayerType = Layer["type"].GetString();

        if (StringEquals(LayerType, TILE_LAYER)) 
        {
            tile_layer *TileLayer = Map->TileLayers + TileLayerIndex;

            TileLayer->StartX = Layer["startx"].GetInt();
            TileLayer->StartY = Layer["starty"].GetInt();
            
            TileLayer->Width = Layer["width"].GetUint();
            TileLayer->Height = Layer["height"].GetUint();

            const Value& Chunks = Layer["chunks"];
            assert(Chunks.IsArray());

            TileLayer->ChunkCount = Chunks.Size();
            TileLayer->Chunks = PushArray<map_chunk>(Arena, TileLayer->ChunkCount);

            for (SizeType ChunkIndex = 0; ChunkIndex < Chunks.Size(); ++ChunkIndex)
            {
                const Value& Chunk = Chunks[ChunkIndex];

                map_chunk *MapChunk = TileLayer->Chunks + ChunkIndex;

                MapChunk->X = Chunk["x"].GetInt();
                MapChunk->Y = Chunk["y"].GetInt();

                MapChunk->Width = Chunk["width"].GetUint();
                MapChunk->Height = Chunk["height"].GetUint();

                const Value& ChunkData = Chunk["data"];
                assert(ChunkData.IsArray());

                MapChunk->GIDCount = ChunkData.Size();
                MapChunk->GIDs = PushArray<u32>(Arena, MapChunk->GIDCount);

                for (SizeType ChunkDataIndex = 0; ChunkDataIndex < ChunkData.Size(); ++ChunkDataIndex)
                {
                    MapChunk->GIDs[ChunkDataIndex] = ChunkData[ChunkDataIndex].GetUint();
                }
            }

            ++TileLayerIndex;
        }
        else if (StringEquals(LayerType, OBJECT_LAYER)) 
        {
            object_layer *ObjectLayer = Map->ObjectLayers + ObjectLayerIndex;

            const Value& Objects = Layer["objects"];
            assert(Objects.IsArray());

            ObjectLayer->ObjectCount = Objects.Size();
            ObjectLayer->Objects = PushArray<map_object>(Arena, ObjectLayer->ObjectCount);

            for (SizeType ObjectIndex = 0; ObjectIndex < Objects.Size(); ++ObjectIndex) 
            {
                const Value& Object = Objects[ObjectIndex];

                map_object *MapObject = ObjectLayer->Objects + ObjectIndex;

                MapObject->X = Object["x"].GetFloat();
                MapObject->Y = Object["y"].GetFloat();

                MapObject->Width = Object["width"].GetFloat();
                MapObject->Height = Object["height"].GetFloat();

                MapObject->Rotation = Object["rotation"].GetFloat();
                MapObject->GID = Object["gid"].GetUint();
                MapObject->ID = Object["id"].GetUint();
            }

            ++ObjectLayerIndex;
        }
        else
        {
            InvalidCodePath;
        }
    }
}
