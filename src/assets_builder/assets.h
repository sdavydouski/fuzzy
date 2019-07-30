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

struct glyph
{
    vec2 SpriteSize;
    vec2 CharacterSize;
    vec2 UV;
    vec2 Alignment;
};

struct font_asset
{
    bitmap TextureAtlas;

    i32 VerticalAdvance;
    i32 Ascent;
    i32 Descent;

    u32 CodepointsRangeCount;
    codepoints_range *CodepointsRanges;

    u32 HorizontalAdvanceTableCount;
    f32 *HorizontalAdvanceTable;

    u32 GlyphCount;
    glyph *Glyphs;
};

struct font_asset_header {
    i32 TextureAtlasWidth;
    i32 TextureAtlasHeight;
    i32 TextureAtlasChannels;
    u64 TextureAtlasOffset;

    i32 VerticalAdvance;
    i32 Ascent;
    i32 Descent;

    u32 CodepointsRangeCount;
    u64 CodepointsRangesOffset;

    u32 HorizontalAdvanceTableCount;
    u64 HorizontalAdvanceTableOffset;

    u32 GlyphCount;
    u64 GlyphsOffset;
};

struct asset_header
{
    i32 MagicValue;
    i32 Version;

    u32 FontCount;
    u64 FontsOffset;
};