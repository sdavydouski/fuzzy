#pragma once

#include "fuzzy_types.h"
#include "fuzzy_tiled.h"

struct codepoints_range
{
    u32 Start;
    u32 End;
    u32 Count;
};

inline u32
GetCodepointsRangeCount(codepoints_range *Range)
{
    u32 Result = (Range->End - Range->Start) + 1;

    assert(Result > 0);

    return Result;
}

struct font_info
{
    bitmap TextureAtlas;

    s32 VerticalAdvance;
    //s32 Ascent;
    //s32 Descent;

    u32 HorizontalAdvanceTableCount;
    f32 *HorizontalAdvanceTable;

    u32 CodepointsRangeCount;
    codepoints_range *CodepointsRanges;
};

// todo: just glyph?
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
    //s32 Ascent;
    //s32 Descent;

    u32 HorizontalAdvanceTableCount;
    u64 HorizontalAdvanceTable;

    u32 CodepointsRangeCount;
    u64 CodepointsRanges;

    u32 GlyphCount;
    u64 Glyphs;
};

struct font_asset
{
    font_info FontInfo;

    u32 GlyphCount;
    glyph_info *Glyphs;
};