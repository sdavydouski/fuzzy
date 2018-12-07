#include <fstream>
#include <vector>
#include <map>

#include <nlohmann\json.hpp>

#include "fuzzy_types.h"
#include "fuzzy.h"

const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

// todo: probably should replace json library; get rid of std::vector

struct rawTileLayer {
    vector<u32> data;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    string name;
    string type;
    u32 opacity;
    b32 visible;
};

struct rawObject {
    u32 gid;
    u32 id;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    string name;
    string type;
    f32 rotation;
    b32 visible;
};

struct rawObjectLayer {
    u32 ObjectsCount;
    vector<rawObject> objects;

    u32 x;
    u32 y;
    string name;
    string type;
    string draworder;
    u32 opacity;
    b32 visible;
};

void from_json(const json& j, rawTileLayer& layer) {
    layer.data = j.at("data").get<vector<u32>>();
    layer.x = j.at("x").get<u32>();
    layer.y = j.at("y").get<u32>();
    layer.width = j.at("width").get<u32>();
    layer.height = j.at("height").get<u32>();
    layer.name = j.at("name").get<string>();
    layer.type = j.at("type").get<string>();
    layer.opacity = j.at("opacity").get<u32>();
    layer.visible = j.at("visible").get<b32>();
}

void from_json(const json& j, rawObject& object) {
    object.id = j.at("id").get<u32>();
    object.x = j.at("x").get<u32>();
    object.y = j.at("y").get<u32>();
    object.width = j.at("width").get<u32>();
    object.height = j.at("height").get<u32>();
    object.name = j.at("name").get<string>();
    object.type = j.at("type").get<string>();
    object.rotation = j.at("rotation").get<f32>();
    object.visible = j.at("visible").get<b32>();

    if (j.find("gid") != j.end()) {
        object.gid = j.at("gid").get<u32>();
    }
    else {
        object.gid = 0;
    }
}

void from_json(const json& j, rawObjectLayer& layer) {
    layer.objects = j.at("objects").get<vector<rawObject>>();
    layer.x = j.at("x").get<u32>();
    layer.y = j.at("y").get<u32>();
    layer.name = j.at("name").get<string>();
    layer.type = j.at("type").get<string>();
    layer.draworder = j.at("draworder").get<string>();
    layer.opacity = j.at("opacity").get<u32>();
    layer.visible = j.at("visible").get<b32>();
}

tile_layer ParseTileLayer(memory_arena* Arena, const rawTileLayer& Layer, const tileset& Tileset, vec2 Scale) {
    tile_layer TileLayer = {};

    for (u32 y = 0; y < Layer.height; ++y) {
        for (u32 x = 0; x < Layer.width; ++x) {
            u32 gid = Layer.data[x + y * Layer.width];

            if (gid > 0) {
                ++TileLayer.TilesCount;
            }
        }
    }

    TileLayer.Tiles = PushArray<tile>(Arena, TileLayer.TilesCount);

    u32 TilesIndex = 0;
    for (u32 y = 0; y < Layer.height; ++y) {
        for (u32 x = 0; x < Layer.width; ++x) {
            u32 Gid = Layer.data[x + y * Layer.width];

            if (Gid > 0) {
                tile Tile = {};

                Tile.Position = { x * Tileset.TileSize.x * Scale.x, y * Tileset.TileSize.y * Scale.y };
                // take three most significant bits
                Tile.Flipped = Gid & (7 << 29);

                // Clear the flags
                Gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

                s32 uvX = (Gid - 1) % Tileset.Columns;
                s32 uvY = (Gid - 1) / Tileset.Columns;

                Tile.UV = { (uvX * (Tileset.TileSize.x + Tileset.Spacing) + Tileset.Margin) / Tileset.ImageSize.x,
                            (uvY * (Tileset.TileSize.y + Tileset.Spacing) + Tileset.Margin) / Tileset.ImageSize.y };

                TileLayer.Tiles[TilesIndex++] = Tile;
            }
        }
    }

    return TileLayer;
}

object_layer ParseObjectLayer(memory_arena* Arena, const rawObjectLayer& Layer, tileset& Tileset, vec2 Scale) {
    object_layer ObjectLayer = {};

    for (u32 i = 0; i < Layer.objects.size(); i++) {
        auto& object = Layer.objects[i];

        if (object.gid != 0) {
            ++ObjectLayer.DrawableEntitiesCount;
        }
        else {
            ++ObjectLayer.EntitiesCount;
        }
    }

    ObjectLayer.Entities = PushArray<entity>(Arena, ObjectLayer.EntitiesCount);
    ObjectLayer.DrawableEntities = PushArray<drawable_entity>(Arena, ObjectLayer.DrawableEntitiesCount);

    u32 EntityIndex = 0;
    u32 DrawableEntityIndex = 0;
    u32 Index = 0;
    for (u32 i = 0; i < Layer.objects.size(); i++) {
        auto& object = Layer.objects[i];

        if (object.gid != 0) {
            drawable_entity Entity = {};

            Entity.Id = object.id;

            if (object.type == "reflector") {
                Entity.Type = entity_type::REFLECTOR;
            }
            else if (object.type == "lamp") {
                Entity.Type = entity_type::LAMP;
            }
            else if (object.type == "platform") {
                Entity.Type = entity_type::PLATFORM;
            }
            else {
                Entity.Type = entity_type::UNKNOWN;
            }
            Entity.Rotation = object.rotation;
            // todo: handle negative angles?
            if (Entity.Rotation == -90.f) {
                Entity.Rotation = 270.f;
            }

            // adjusting position
            // tile objects have their position at bottom-left
            // see: https://github.com/bjorn/tiled/issues/91
            if (Entity.Rotation == 0.f) {
                Entity.Position = { (f32)object.x * Scale.x, ((f32)object.y - Tileset.TileSize.y) * Scale.y };
            }
            else if (Entity.Rotation == 90.f) {
                Entity.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            }
            else if (Entity.Rotation == 180.f) {
                Entity.Position = { ((f32)object.x - Tileset.TileSize.x) * Scale.x, (f32)object.y * Scale.y };
            }
            else if (Entity.Rotation == 270.f) {
                Entity.Position = { ((f32)object.x - Tileset.TileSize.x) * Scale.x, ((f32)object.y - Tileset.TileSize.y) * Scale.y };
            }

            u32 gid = object.gid;

            Entity.Flipped = gid & (7 << 29);       // take three most significant bits

            gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

            s32 uvX = (gid - 1) % Tileset.Columns;
            s32 uvY = (gid - 1) / Tileset.Columns;

            Entity.UV = { (uvX * (Tileset.TileSize.x + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.ImageSize.x,
                         (uvY * (Tileset.TileSize.y + Tileset.Spacing) + Tileset.Margin) / (f32)Tileset.ImageSize.y };


            Entity.Offset = Index * sizeof(drawable_entity);
            Entity.SpriteScale = vec2(1.f);
            Entity.ShouldRender = 1;
            Entity.Collides = true;
            Entity.UnderEffect = false;

            tile_spec* Spec = GetOrCreateTileSpec(&Tileset, gid, 0);
            
            if (Spec) {
                // todo: why do i need this???
                Entity.Box.Position = Entity.Position + Spec->Box.Position * Scale;
                Entity.Box.Size = Spec->Box.Size * Scale;
            }
            else {
                Entity.Box.Position = Entity.Position;
                Entity.Box.Size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };
            }

            ++Index;

            ObjectLayer.DrawableEntities[DrawableEntityIndex++] = Entity;
        }
        else {
            entity Entity = {};
            Entity.Id = object.id;
            Entity.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            Entity.Box.Position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            Entity.Box.Size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };

            ObjectLayer.Entities[EntityIndex++] = Entity;
        }
    }

    return ObjectLayer;
}

tileset LoadTileset(platform_read_json_file* ReadJsonFile, const string& Path, memory_arena* Arena) {
    tileset Tileset = {};
    // todo: think about size
    Tileset.TileSpecsCount = 10;
    Tileset.TilesHashTable = PushArray<tile_spec>(Arena, Tileset.TileSpecsCount);

    json TilesetInfo = ReadJsonFile(Path);

    Tileset.Columns = TilesetInfo["columns"];
    Tileset.Margin = TilesetInfo["margin"];
    Tileset.Spacing = TilesetInfo["spacing"];
    Tileset.TileSize = { TilesetInfo["tilewidth"], TilesetInfo["tileheight"] };
    Tileset.ImageSize = { TilesetInfo["imagewidth"], TilesetInfo["imageheight"] };

    for (auto& It = TilesetInfo["tiles"].begin(); It != TilesetInfo["tiles"].end(); ++It) {
        auto& Value = It.value();

        // todo: 
        //if (value.find("type") != value.end()) {
        //    tileSpec spec = {};
        //    u32 gid = (u32)std::stoi(it.key()) + 1;

        //    spec.type = value["type"];

        //    tileset.tiles.emplace(gid, spec);
        //}

        // todo: wtf is this???
        if (Value.find("objectgroup") != Value.end()) {
            u32 Gid = (u32)std::stoi(It.key()) + 1;

            tile_spec* Spec = GetOrCreateTileSpec(&Tileset, Gid, Arena);

            Spec->Box.Position = {
                Value["objectgroup"]["objects"][0]["x"],
                Value["objectgroup"]["objects"][0]["y"]
            };
            Spec->Box.Size = {
                Value["objectgroup"]["objects"][0]["width"],
                Value["objectgroup"]["objects"][0]["height"]
            };
        }
    }

    return Tileset;
}

tiled_map LoadMap(
    memory_arena* Arena, 
    platform_read_json_file* ReadJsonFile, 
    const string& Path, 
    tileset& Tileset, 
    vec2 Scale
) {
    tiled_map Map = {};

    json MapInfo = ReadJsonFile(Path);

    Map.Width = MapInfo["width"];
    Map.Height = MapInfo["height"];

    auto Layers = MapInfo["layers"];
    for (const auto& Layer : Layers) {
        string Type = Layer["type"];

        if (Type == "tilelayer") {
            ++Map.TileLayersCount;
        }
        else if (Type == "objectgroup") {
            ++Map.ObjectLayersCount;
        }
    }

    Map.TileLayers = PushArray<tile_layer>(Arena, Map.TileLayersCount);
    Map.ObjectLayers = PushArray<object_layer>(Arena, Map.ObjectLayersCount);

    u32 TileLayersIndex = 0;
    u32 ObjectLayersIndex = 0;
    for (const auto& Layer : Layers) {
        string Type = Layer["type"];

        if (Type == "tilelayer") {
            Map.TileLayers[TileLayersIndex++] = ParseTileLayer(Arena, Layer, Tileset, Scale);
        }
        else if (Type == "objectgroup") {
            Map.ObjectLayers[ObjectLayersIndex++] = ParseObjectLayer(Arena, Layer, Tileset, Scale);
        }
    }

    return Map;
}
