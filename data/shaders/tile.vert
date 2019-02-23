#version 330 core

#pragma optimize(off)
#pragma debug(on)

// <vec2 - position, vec2 - uv>
//layout(location = 0) in vec4 in_Vertex;
layout(location = 0) in vec4 in_Vertex;
layout(location = 1) in mat4 in_InstanceModel;
layout(location = 5) in vec2 in_InstanceUVOffset;
//layout(location = 1) in vec4 tile;          // <vec2 - position, vec2 - uv>
//layout(location = 2) in uint tileFlipped;

//// todo: combine this differently
//layout(location = 3) in vec2 position;
////layout(location = 4) in vec4 aabb;
//layout(location = 5) in vec3 uvr;
//layout(location = 6) in uint flipped;
//layout(location = 7) in vec2 spriteScale;
//layout(location = 8) in uint shouldRender;

//layout(location = 9) in vec4 particleBox;
//layout(location = 10) in vec2 particleUv;
//layout(location = 11) in float particleAlpha;

out vec2 uv;
out vec2 instanceUVOffset;
//out vec2 uvOffset;
//out float alpha;

//uniform int type;
//uniform vec2 spriteSize;

//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;

//#define ROTATE_90 1.f
//#define ROTATE_180 2.f
//#define ROTATE_270 3.f

//#define TILE_TYPE 1.f
//#define SPRITE_TYPE 2.f
//#define ENTITY_TYPE 3.f
//#define PARTICLE_TYPE 4.f

//uint FLIPPED_HORIZONTALLY_FLAG = 0x80000000u;
//uint FLIPPED_VERTICALLY_FLAG = 0x40000000u;
//uint FLIPPED_DIAGONALLY_FLAG = 0x20000000u;

//vec2 swapXY(vec2 vec) {
//    float temp = vec.x;
//    vec.x = vec.y;
//    vec.y = temp;
    
//    return vec;
//}

//#define PI 3.14159265

//float toRadians(float angleInDegrees) {
//    return angleInDegrees * PI / 180.f;    
//}

// view-projection matrix
uniform mat4 u_VP;
uniform vec2 u_TileSize;

void main()
{
    // getting correct uv from texture atlas
    uv = in_Vertex.zw * u_TileSize;
    instanceUVOffset = in_InstanceUVOffset;

    vec2 position = in_Vertex.xy;
    gl_Position = u_VP * in_InstanceModel * vec4(position, 0.f, 1.f);

    //uv = spriteSize * sprite.zw;
    
    //if (type == TILE_TYPE) {

    //    bool flippedHorizontally = bool(tileFlipped & FLIPPED_HORIZONTALLY_FLAG);
    //    bool flippedVertically = bool(tileFlipped & FLIPPED_VERTICALLY_FLAG);
    //    bool flippedDiagonally = bool(tileFlipped & FLIPPED_DIAGONALLY_FLAG);

    //    // handle rotating
    //    if (flippedHorizontally) {
    //        uv.x = spriteSize.x - uv.x;
    //    }
    //    if (flippedVertically) {
    //        uv.y = spriteSize.y - uv.y;
    //    }
    //    if (flippedDiagonally) {
    //        uv = swapXY(uv);
    //    }

    //    uvOffset = tile.zw;
    //    alpha = 1.f;

    //    gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(tile.xy, 0.f, 0.f));
    //} else if (type == SPRITE_TYPE || type == ENTITY_TYPE) {
    //    if (shouldRender != 1u) return;
        
    //    uvOffset = uvr.xy;
    //    alpha = 1.f;
        
    //    bool flippedHorizontally = bool(flipped & FLIPPED_HORIZONTALLY_FLAG);
    //    bool flippedVertically = bool(flipped & FLIPPED_VERTICALLY_FLAG);
    //    bool flippedDiagonally = bool(flipped & FLIPPED_DIAGONALLY_FLAG);
        
    //    if (flippedHorizontally) {
    //        uv.x = spriteSize.x - uv.x;
    //    }
    //    if (flippedVertically) {
    //        uv.y = spriteSize.y - uv.y;
    //    }
    //    if (flippedDiagonally) {
    //        uv = swapXY(uv);
    //    }

    //    uv *= spriteScale;

    //    //vec2 position = aabb.xy;
    //    //vec2 size = aabb.zw;

    //    float rotate = uvr.z;

    //    mat4 translationMatrix = mat4(
    //        1.f, 0.f, 0.f, 0.f,                 // first column (not row!)
    //        0.f, 1.f, 0.f, 0.f,
    //        0.f, 0.f, 1.f, 0.f,
    //        position.x, position.y, 0.f, 1.f
    //    );

    //    vec2 spriteSize = vec2(model[0][0] * spriteScale.x, model[1][1] * spriteScale.y);
        
    //    mat4 preRotationTranslationMatrix = mat4(
    //        1.f, 0.f, 0.f, 0.f,
    //        0.f, 1.f, 0.f, 0.f,
    //        0.f, 0.f, 1.f, 0.f,
    //        spriteSize.x / 2.f, spriteSize.y / 2.f, 0.f, 1.f
    //    );

    //    mat4 postRotationTranslationMatrix = mat4(
    //        1.f, 0.f, 0.f, 0.f,
    //        0.f, 1.f, 0.f, 0.f,
    //        0.f, 0.f, 1.f, 0.f,
    //        -spriteSize.x / 2.f, -spriteSize.y / 2.f, 0.f, 1.f
    //    );
        
    //    mat4 rotationMatrix = preRotationTranslationMatrix * mat4(
    //        cos(toRadians(rotate)), sin(toRadians(rotate)), 0.f, 0.f,
    //        -sin(toRadians(rotate)), cos(toRadians(rotate)), 0.f, 0.f,
    //        0.f, 0.f, 1.f, 0.f,
    //        0.f, 0.f, 0.f, 1.f
    //    ) * postRotationTranslationMatrix;

    //    mat4 scalingMatrix = mat4(
    //       spriteSize.x, 0.f, 0.f, 0.f,     
    //       0.f, spriteSize.y, 0.f, 0.f,
    //       0.f, 0.f, 1.f, 0.f,
    //       0.f, 0.f, 0.f, 1.f
    //    );   // todo: same as uniform model

    //    mat4 model = translationMatrix * rotationMatrix * scalingMatrix;

    //    gl_Position = projection * view * model * vec4(sprite.xy, 0.f, 1.0f);
    //} else if (type == PARTICLE_TYPE) {
    //    uvOffset = particleUv;
    //    alpha = particleAlpha;

    //    vec2 particlePosition = particleBox.xy;
    //    vec2 particleSize = particleBox.zw;

    //    mat4 scalingMatrix = mat4(
    //       particleSize.x, 0.f, 0.f, 0.f,     
    //       0.f, particleSize.y, 0.f, 0.f,
    //       0.f, 0.f, 1.f, 0.f,
    //       0.f, 0.f, 0.f, 1.f
    //    );
        
    //    mat4 model = scalingMatrix;

    //    gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(particlePosition, 0.f, 0.f));
    //}
}
