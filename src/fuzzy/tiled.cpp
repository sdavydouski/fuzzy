#include <fstream>
#include <vector>
#include <map>

#include <nlohmann\json.hpp>

#include "fuzzy_types.h"
#include "fuzzy.h"

const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

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

tile_layer ParseTileLayer(memory_arena* Arena, const rawTileLayer& layer, const tileset& tileset, vec2 scale);
object_layer ParseObjectLayer(memory_arena* Arena, const rawObjectLayer& layer, const tileset& tileset, vec2 scale);

tileset loadTileset(platform_read_json_file* ReadJsonFile, const string& path) {
    tileset tileset = {};

    json tilesetInfo = ReadJsonFile(path);

    tileset.columns = tilesetInfo["columns"];
    tileset.margin = tilesetInfo["margin"];
    tileset.spacing = tilesetInfo["spacing"];
    tileset.tileSize = { tilesetInfo["tilewidth"], tilesetInfo["tileheight"] };
    tileset.imageSize = { tilesetInfo["imagewidth"], tilesetInfo["imageheight"] };

    for (auto& it = tilesetInfo["tiles"].begin(); it != tilesetInfo["tiles"].end(); ++it) {
        auto& value = it.value();

        // todo: 
        //if (value.find("type") != value.end()) {
        //    tileSpec spec = {};
        //    u32 gid = (u32)std::stoi(it.key()) + 1;

        //    spec.type = value["type"];

        //    tileset.tiles.emplace(gid, spec);
        //}

        if (value.find("objectgroup") != value.end()) {
            tile_spec spec = {};
            u32 gid = (u32)std::stoi(it.key()) + 1;

            spec.box.position = {
                value["objectgroup"]["objects"][0]["x"],
                value["objectgroup"]["objects"][0]["y"]
            };
            spec.box.size = {
                value["objectgroup"]["objects"][0]["width"],
                value["objectgroup"]["objects"][0]["height"]
            };

            tileset.tiles.emplace(gid, spec);
        }
    }

    return tileset;
}

tiled_map LoadMap(
    memory_arena* Arena, 
    platform_read_json_file* ReadJsonFile, 
    const string& Path, 
    const tileset& Tileset, 
    const vec2 Scale
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

tile_layer ParseTileLayer(memory_arena* Arena, const rawTileLayer& layer, const tileset& tileset, vec2 scale) {
    tile_layer TileLayer = {};

    for (u32 y = 0; y < layer.height; ++y) {
        for (u32 x = 0; x < layer.width; ++x) {
            u32 gid = layer.data[x + y * layer.width];

            if (gid > 0) {
                ++TileLayer.TilesCount;
            }
        }
    }

    TileLayer.Tiles = PushArray<tile>(Arena, TileLayer.TilesCount);

    u32 TilesIndex = 0;
    for (u32 y = 0; y < layer.height; ++y) {
        for (u32 x = 0; x < layer.width; ++x) {
            u32 gid = layer.data[x + y * layer.width];

            if (gid > 0) {
                tile tile = {};

                tile.position = { x * tileset.tileSize.x * scale.x, y * tileset.tileSize.y * scale.y };
                // take three most significant bits
                tile.flipped = gid & (7 << 29);

                // Clear the flags
                gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

                s32 uvX = (gid - 1) % tileset.columns;
                s32 uvY = (gid - 1) / tileset.columns;

                tile.uv = { (uvX * (tileset.tileSize.x + tileset.spacing) + tileset.margin) / tileset.imageSize.x,
                           (uvY * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / tileset.imageSize.y };

                TileLayer.Tiles[TilesIndex++] = tile;
            }
        }
    }

    return TileLayer;
}

object_layer ParseObjectLayer(memory_arena* Arena, const rawObjectLayer& Layer, const tileset& Tileset, vec2 Scale) {
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
    u32 index = 0;
    for (u32 i = 0; i < Layer.objects.size(); i++) {
        auto& object = Layer.objects[i];

        if (object.gid != 0) {
            drawable_entity entity = {};

            entity.id = object.id;

            if (object.type == "reflector") {
                entity.type = entity_type::REFLECTOR;
            }
            else if (object.type == "lamp") {
                entity.type = entity_type::LAMP;
            }
            else if (object.type == "platform") {
                entity.type = entity_type::PLATFORM;
            }
            else {
                entity.type = entity_type::UNKNOWN;
            }
            entity.rotation = object.rotation;
            // todo: handle negative angles?
            if (entity.rotation == -90.f) {
                entity.rotation = 270.f;
            }

            // adjusting position
            // tile objects have their position at bottom-left
            // see: https://github.com/bjorn/tiled/issues/91
            if (entity.rotation == 0.f) {
                entity.position = { (f32)object.x * Scale.x, ((f32)object.y - Tileset.tileSize.y) * Scale.y };
            }
            else if (entity.rotation == 90.f) {
                entity.position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            }
            else if (entity.rotation == 180.f) {
                entity.position = { ((f32)object.x - Tileset.tileSize.x) * Scale.x, (f32)object.y * Scale.y };
            }
            else if (entity.rotation == 270.f) {
                entity.position = { ((f32)object.x - Tileset.tileSize.x) * Scale.x, ((f32)object.y - Tileset.tileSize.y) * Scale.y };
            }

            u32 gid = object.gid;

            entity.flipped = gid & (7 << 29);       // take three most significant bits

            gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

            s32 uvX = (gid - 1) % Tileset.columns;
            s32 uvY = (gid - 1) / Tileset.columns;

            entity.uv = { (uvX * (Tileset.tileSize.x + Tileset.spacing) + Tileset.margin) / (f32)Tileset.imageSize.x,
                         (uvY * (Tileset.tileSize.y + Tileset.spacing) + Tileset.margin) / (f32)Tileset.imageSize.y };


            entity.offset = index * sizeof(drawable_entity);
            entity.spriteScale = vec2(1.f);
            entity.shouldRender = 1;
            entity.collides = true;
            entity.underEffect = false;

            if (Tileset.tiles.find(gid) != Tileset.tiles.end()) {
                tile_spec spec = Tileset.tiles.at(gid);

                entity.box.position = entity.position + spec.box.position * Scale;
                entity.box.size = spec.box.size * Scale;
            }
            else {
                entity.box.position = entity.position;
                entity.box.size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };
            }

            ++index;

            ObjectLayer.DrawableEntities[DrawableEntityIndex++] = entity;
        }
        else {
            entity entity = {};
            entity.id = object.id;
            entity.position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            entity.box.position = { (f32)object.x * Scale.x, (f32)object.y * Scale.y };
            entity.box.size = { (f32)object.width * Scale.x, (f32)object.height * Scale.y };

            ObjectLayer.Entities[EntityIndex++] = entity;
        }
    }

    return ObjectLayer;
}
