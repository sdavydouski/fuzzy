#pragma once

#include "fuzzy_types.h"
#include "fuzzy_tiled.h"

struct font_range
{
    s32 CodepointStart;
    s32 CodepointEnd;
};

inline s32
GetFontRangeCount(font_range *Range)
{
    s32 Result = Range->CodepointEnd - Range->CodepointStart + 1;

    assert(Result > 0);

    return Result;
}

// todo: save as normalised(0-1 range) coordinates?
struct font_info
{
    bitmap TextureAtlas;

    s32 VerticalAdvance;
    s32 Ascent;
    s32 Descent;

    u32 HorizontalAdvanceTableCount;
    f32 *HorizontalAdvanceTable;
};

struct glyph_info
{
    vec2 SpriteSize;
    vec2 CharacterSize;
    // top-left
    vec2 UV;
    vec2 Alignment;
};

// todo: generalize to more assets (not just font)
struct font_asset_header
{
    s32 MagicValue;
    s32 Version;
    
    s32 TextureAtlasWidth;
    s32 TextureAtlasHeight;
    s32 TextureAtlasChannels;
    u64 TextureAtlas;

    s32 VerticalAdvance;
    s32 Ascent;
    s32 Descent;

    u32 HorizontalAdvanceTableCount;
    u64 HorizontalAdvanceTable;

    u32 GlyphCount;
    u64 Glyphs;
};

struct font_asset
{
    font_info FontInfo;

    u32 GlyphCount;
    glyph_info *Glyphs;
};