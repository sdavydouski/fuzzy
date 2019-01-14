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

    string TilesetJson = Platform->ReadTextFile(FullTilesetPath);

    Map->Tilesets[0].Source = {};
    LoadTileset(&Map->Tilesets[0].Source, TilesetJson.c_str(), Arena, Platform);

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

                MapObject->X = Object["x"].GetInt();
                MapObject->Y = Object["y"].GetInt();

                MapObject->Width = Object["width"].GetUint();
                MapObject->Height = Object["height"].GetUint();

                MapObject->Rotation = Object["rotation"].GetInt();
                MapObject->GID = Object["gid"].GetUint();
            }

            ++ObjectLayerIndex;
        }
        else
        {
            InvalidCodePath;
        }
    }
}

//#include <fstream>
//#include <vector>
//#include <map>
//
//#include <nlohmann\json.hpp>
//
//#include "fuzzy_types.h"
//#include "fuzzy.h"
//
//
//// todo: probably should replace json library; get rid of std::vector
//
//struct rawTileLayer {
//    vector<u32> data;
//    u32 x;
//    u32 y;
//    u32 width;
//    u32 height;
//    string name;
//    string type;
//    u32 opacity;
//    b32 visible;
//};
//
//struct rawObject {
//    u32 gid;
//    u32 id;
//    u32 x;
//    u32 y;
//    u32 width;
//    u32 height;
//    string name;
//    string type;
//    f32 rotation;
//    b32 visible;
//};
//
//struct rawObjectLayer {
//    u32 ObjectsCount;
//    vector<rawObject> objects;
//
//    u32 x;
//    u32 y;
//    string name;
//    string type;
//    string draworder;
//    u32 opacity;
//    b32 visible;
//};
//
//void from_json(const json& j, rawTileLayer& layer) {
//    layer.data = j.at("data").get<vector<u32>>();
//    layer.x = j.at("x").get<u32>();
//    layer.y = j.at("y").get<u32>();
//    layer.width = j.at("width").get<u32>();
//    layer.height = j.at("height").get<u32>();
//    layer.name = j.at("name").get<string>();
//    layer.type = j.at("type").get<string>();
//    layer.opacity = j.at("opacity").get<u32>();
//    layer.visible = j.at("visible").get<b32>();
//}
//
//void from_json(const json& j, rawObject& object) {
//    object.id = j.at("id").get<u32>();
//    object.x = j.at("x").get<u32>();
//    object.y = j.at("y").get<u32>();
//    object.width = j.at("width").get<u32>();
//    object.height = j.at("height").get<u32>();
//    object.name = j.at("name").get<string>();
//    object.type = j.at("type").get<string>();
//    object.rotation = j.at("rotation").get<f32>();
//    object.visible = j.at("visible").get<b32>();
//
//    if (j.find("gid") != j.end()) {
//        object.gid = j.at("gid").get<u32>();
//    }
//    else {
//        object.gid = 0;
//    }
//}
//
//void from_json(const json& j, rawObjectLayer& layer) {
//    layer.objects = j.at("objects").get<vector<rawObject>>();
//    layer.x = j.at("x").get<u32>();
//    layer.y = j.at("y").get<u32>();
//    layer.name = j.at("name").get<string>();
//    layer.type = j.at("type").get<string>();
//    layer.draworder = j.at("draworder").get<string>();
//    layer.opacity = j.at("opacity").get<u32>();
//    layer.visible = j.at("visible").get<b32>();
//}
//
//tile_layer ParseTileLayer(memory_arena *Arena, const rawTileLayer& Layer, const tileset& Tileset, vec2 Scale) {
//    tile_layer TileLayer = {};
//
//    for (u32 y = 0; y < Layer.height; ++y) {
//        for (u32 x = 0; x < Layer.width; ++x) {
//            u32 gid = Layer.data[x + y * Layer.width];
//
//            if (gid > 0) {
//                ++TileLayer.TilesCount;
//            }
//        }
//    }
//
//    TileLayer.Tiles = PushArray<tile>(Arena, TileLayer.TilesCount);
//
//    u32 TilesIndex = 0;
//    for (u32 y = 0; y < Layer.height; ++y) {
//        for (u32 x = 0; x < Layer.width; ++x) {
//            u32 Gid = Layer.data[x + y * Layer.width];
//
//            if (Gid > 0) {
//                tile Tile = {};
//
//                Tile.Position = { x * Tileset.TileSize.x * Scale.x, y * Tileset.TileSize.y * Scale.y };
//                // take three most significant bits
//                Tile.Flipped = Gid & (7 << 29);
//
//                // Clear the flags
//                Gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);
//
//                s32 uvX = (Gid - 1) % Tileset.Columns;
//                s32 uvY = (Gid - 1) / Tileset.Columns;
//
//                //Tile.UV = { (uvX * (Tileset.TileSize.x + Tileset.Spacing) + Tileset.Margin) / Tileset.ImageSize.x,
//                //            (uvY * (Tileset.TileSize.y + Tileset.Spacing) + Tileset.Margin) / Tileset.ImageSize.y };
//
//                TileLayer.Tiles[TilesIndex++] = Tile;
//            }
//        }
//    }
//
//    return TileLayer;
//}
//
//object_layer ParseObjectLayer(memory_arena *Arena, const rawObjectLayer& Layer, tileset& Tileset, vec2 Scale) {
//    object_layer ObjectLayer = {};
//
//    for (u32 i = 0; i < Layer.objects.size(); i++) {
//        auto& object = Layer.objects[i];
//
//        if (object.gid != 0) {
//            ++ObjectLayer.DrawableEntitiesCount;
//        }
//        else {
//            ++ObjectLayer.EntitiesCount;
//        }
//    }
//
//    ObjectLayer.Entities = PushArray<entity>(Arena, ObjectLayer.EntitiesCount);
//    ObjectLayer.DrawableEntities = PushArray<drawable_entity>(Arena, ObjectLayer.DrawableEntitiesCount);
//
//    u32 EntityIndex = 0;
//    u32 DrawableEntityIndex = 0;
//    u32 Index = 0;
//    for (u32 i = 0; i < Layer.objects.size(); i++) {
//        auto& object = Layer.objects[i];
//
//        if (object.gid != 0) {
//            drawable_entity Entity = {};
//
//            Entity.Id = object.id;
//
//            if (object.type == "reflector") {
//                Entity.Type = tile_type::REFLECTOR;
//            }
//            else if (object.type == "lamp") {
//                Entity.Type = tile_type::LAMP;
//            }
//            else if (object.type == "platform") {
//                Entity.Type = tile_type::PLATFORM;
//            }
//            else {
//                Entity.Type = tile_type::UNKNOWN;
//            }
//            Entity.Rotation = object.rotation;
//            // todo: handle negative angles?
//            if (Entity.Rotation == -90.f) {
//                Entity.Rotation = 270.f;
//            }
//
//            // adjusting position
//            // tile objects have their position at bottom-left
//            // see: https://github.com/bjorn/tiled/issues/91
//            if (Entity.Rotation == 0.f) {
//                Entity.Position = { (f32)object.x * Scale.x, ((f32)object.y - Tileset.TileSize.y) * Scale.y };
//            }
//            else if (Entity.Rotation == 90.f) {
//                Entity.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
//            }
//            else if (Entity.Rotation == 180.f) {
//                Entity.Position = { ((f32)object.x - Tileset.TileSize.x) * Scale.x, (f32)object.y * Scale.y };
//            }
//            else if (Entity.Rotation == 270.f) {
//                Entity.Position = { ((f32)object.x - Tileset.TileSize.x) * Scale.x, ((f32)object.y - Tileset.TileSize.y) * Scale.y };
//            }
//
//            u32 gid = object.gid;
//
//            Entity.Flipped = gid & (7 << 29);       // take three most significant bits
//
//            gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);
//
//            s32 uvX = (gid - 1) % Tileset.Columns;
//            s32 uvY = (gid - 1) / Tileset.Columns;
//
//            //Entity.UV = { (uvX * (Tileset.TileSize.x + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.ImageSize.x,
//            //             (uvY * (Tileset.TileSize.y + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.ImageSize.y };
//
//
//            Entity.Offset = Index * sizeof(drawable_entity);
//            Entity.SpriteScale = vec2(1.f);
//            Entity.ShouldRender = 1;
//            Entity.Collides = true;
//            Entity.UnderEffect = false;
//
//            tile_meta_info* Spec = GetTileMetaInfo(&Tileset, gid);
//            //tile_meta_info* Spec = GetOrCreateTileSpec(&Tileset, gid, 0);
//            
//            if (Spec) {
//                // todo: why do i need this???
//                //Entity.Box.Position = Entity.Position + Spec->Box.Position * Scale;
//                //Entity.Box.Size = Spec->Box.Size * Scale;
//            }
//            else {
//                Entity.Box.Position = Entity.Position;
//                Entity.Box.Size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };
//            }
//
//            ++Index;
//
//            ObjectLayer.DrawableEntities[DrawableEntityIndex++] = Entity;
//        }
//        else {
//            entity Entity = {};
//            Entity.Id = object.id;
//            Entity.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
//            Entity.Box.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
//            Entity.Box.Size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };
//
//            ObjectLayer.Entities[EntityIndex++] = Entity;
//        }
//    }
//
//    return ObjectLayer;
//}
//
//tileset LoadTileset(platform_read_json_file *ReadJsonFile, const string& Path, memory_arena *Arena) {
//    tileset Tileset = {};
//    // todo: think about size
//    Tileset.TileCount = 10;
//    Tileset.Tiles = PushArray<tile_meta_info>(Arena, Tileset.TileCount);
//
//    json TilesetInfo = ReadJsonFile(Path);
//
//    Tileset.Columns = TilesetInfo["columns"];
//    Tileset.Margin = TilesetInfo["margin"];
//    Tileset.Spacing = TilesetInfo["spacing"];
//    Tileset.TileSize = { TilesetInfo["tilewidth"], TilesetInfo["tileheight"] };
//    //Tileset.ImageSize = { TilesetInfo["imagewidth"], TilesetInfo["imageheight"] };
//
//    for (auto& It = TilesetInfo["tiles"].begin(); It != TilesetInfo["tiles"].end(); ++It) {
//        auto& Value = It.value();
//
//        // todo: 
//        //if (value.find("type") != value.end()) {
//        //    tileSpec spec = {};
//        //    u32 gid = (u32)std::stoi(it.key()) + 1;
//
//        //    spec.type = value["type"];
//
//        //    tileset.tiles.emplace(gid, spec);
//        //}
//
//        // todo: wtf is this???
//        if (Value.find("objectgroup") != Value.end()) {
//            u32 Gid = Value["id"] + 1;
//
//            tile_meta_info* Spec = CreateTileMetaInfo(&Tileset, Gid, Arena);
//
//            //Spec->Box.Position = {
//            //    Value["objectgroup"]["objects"][0]["x"],
//            //    Value["objectgroup"]["objects"][0]["y"]
//            //};
//            //Spec->Box.Size = {
//            //    Value["objectgroup"]["objects"][0]["width"],
//            //    Value["objectgroup"]["objects"][0]["height"]
//            //};
//        }
//    }
//
//    return Tileset;
//}
//
//tiled_map LoadMap(
//    memory_arena *Arena, 
//    platform_read_json_file *ReadJsonFile, 
//    const string& Path, 
//    tileset& Tileset, 
//    vec2 Scale
//) {
//    tiled_map Map = {};
//
//    json MapInfo = ReadJsonFile(Path);
//
//    Map.Width = MapInfo["width"];
//    Map.Height = MapInfo["height"];
//
//    auto Layers = MapInfo["layers"];
//    for (const auto& Layer : Layers) {
//        string Type = Layer["type"];
//
//        if (Type == "tilelayer") {
//            ++Map.TileLayersCount;
//        }
//        else if (Type == "objectgroup") {
//            ++Map.ObjectLayersCount;
//        }
//    }
//
//    Map.TileLayers = PushArray<tile_layer>(Arena, Map.TileLayersCount);
//    Map.ObjectLayers = PushArray<object_layer>(Arena, Map.ObjectLayersCount);
//
//    u32 TileLayersIndex = 0;
//    u32 ObjectLayersIndex = 0;
//    for (const auto& Layer : Layers) {
//        string Type = Layer["type"];
//
//        if (Type == "tilelayer") {
//            Map.TileLayers[TileLayersIndex++] = ParseTileLayer(Arena, Layer, Tileset, Scale);
//        }
//        else if (Type == "objectgroup") {
//            Map.ObjectLayers[ObjectLayersIndex++] = ParseObjectLayer(Arena, Layer, Tileset, Scale);
//        }
//    }
//
//    return Map;
//}


