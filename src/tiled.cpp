const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

struct tile {
    vec2 position;
    vec2 uv;
    u32 flipped;
};

struct tileLayer {
    vector<tile> tiles;
};

struct objectLayer {
    vector<entity> entities;
    vector<drawableEntity> drawableEntities;
};

struct map {
    u32 width;
    u32 height;

    vector<tileLayer> tileLayers;
    vector<objectLayer> objectLayers;
};

struct tileset {
    u32 columns;
    u32 margin;
    u32 spacing;
    vec2 tileSize;
    vec2 imageSize;
};

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
    } else {
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

tileLayer parseTileLayer(const rawTileLayer& layer, const tileset& tileset, vec2 scale);
objectLayer parseObjectLayer(const rawObjectLayer& layer, const tileset& tileset, vec2 scale);

tileset loadTileset(const string& path) {
    tileset tileset = {};

    std::fstream tilesetIn(path);
    json tilesetInfo;
    tilesetIn >> tilesetInfo;

    tileset.columns = tilesetInfo["columns"];
    tileset.margin = tilesetInfo["margin"];
    tileset.spacing = tilesetInfo["spacing"];
    tileset.tileSize = { tilesetInfo["tilewidth"], tilesetInfo["tileheight"] };
    tileset.imageSize = { tilesetInfo["imagewidth"], tilesetInfo["imageheight"] };

    return tileset;
}

map loadMap(const string& path, const tileset& tileset, const vec2 scale) {
    map map = {};

    std::fstream mapIn(path);
    json mapInfo;
    mapIn >> mapInfo;

    map.width = mapInfo["width"];
    map.height = mapInfo["height"];

    auto layers = mapInfo["layers"];
    for (const auto& layer : layers) {
        string type = layer["type"];

        if (type == "tilelayer") {
            map.tileLayers.push_back(parseTileLayer(layer, tileset, scale));            
        } else if (type == "objectgroup") {
            map.objectLayers.push_back(parseObjectLayer(layer, tileset, scale));
        }
    }
    
    return map;
}

tileLayer parseTileLayer(const rawTileLayer& layer, const tileset& tileset, vec2 scale) {
    tileLayer tileLayer = {};

    for (u32 y = 0; y < layer.height; ++y) {
        for (u32 x = 0; x < layer.width; ++x) {
            u32 gid = layer.data[x + y * layer.width];

            if (gid > 0) {
                tile tile = {};

                tile.position = {x * tileset.tileSize.x * scale.x, y * tileset.tileSize.y * scale.y};
                // take three most significant bits
                tile.flipped = gid & (7 << 29);

                // Clear the flags
                gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

                s32 uvX = (gid - 1) % tileset.columns;
                s32 uvY = (gid - 1) / tileset.columns;

                tile.uv = {(uvX * (tileset.tileSize.x + tileset.spacing) + tileset.margin) / tileset.imageSize.x, 
                           (uvY * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / tileset.imageSize.y};

                tileLayer.tiles.push_back(tile);
            }
        }
    }

    return tileLayer;
}

objectLayer parseObjectLayer(const rawObjectLayer& layer, const tileset& tileset, vec2 scale) {
    objectLayer objectLayer = {};

    u32 index = 0;
    for (u32 i = 0; i < layer.objects.size(); i++) {
        auto& object = layer.objects[i];
        
        if (object.gid != 0) {
            drawableEntity entity = {};
            if (object.type == "reflector") {
                entity.type = entityType::REFLECTOR;
            } else {
                entity.type = entityType::UNKNOWN;
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
                entity.box.position = { (f32) object.x * scale.x, ((f32) object.y - tileset.tileSize.y) * scale.y };
            } else if (entity.rotation == 90.f) {
                entity.box.position = { (f32) object.x * scale.x, (f32) object.y * scale.y };
            } else if (entity.rotation == 180.f) {
                entity.box.position = { ((f32) object.x - tileset.tileSize.x) * scale.x, (f32) object.y * scale.y };
            } else if (entity.rotation == 270.f) {
                entity.box.position = { ((f32) object.x - tileset.tileSize.x) * scale.x, ((f32) object.y - tileset.tileSize.y) * scale.y };
            }

            u32 gid = object.gid;

            entity.flipped = gid & (7 << 29);       // take three most significant bits

            gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

            s32 uvX = (gid - 1) % tileset.columns;
            s32 uvY = (gid - 1) / tileset.columns;

            entity.uv = {(uvX * (tileset.tileSize.x + tileset.spacing) + tileset.margin) / (f32) tileset.imageSize.x,
                         (uvY * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / (f32) tileset.imageSize.y};

            entity.box.size = {(f32) object.width * scale.x, (f32) object.height * scale.y};
            entity.offset = index * sizeof(drawableEntity);
            entity.spriteScale = vec2(1.f);
            entity.shouldRender = 1;
            entity.collides = true;
            entity.underEffect = false;
            ++index;
            
            objectLayer.drawableEntities.push_back(entity);
        } else {
            entity entity = {};
            entity.box.position = {(f32) object.x * scale.x, (f32) object.y * scale.y};
            entity.box.size = {(f32) object.width * scale.x, (f32) object.height * scale.y};

            objectLayer.entities.push_back(entity);
        }
    }

    return objectLayer;
}