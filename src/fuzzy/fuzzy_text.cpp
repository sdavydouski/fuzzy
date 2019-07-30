#include "assets.h"

internal u32
GetCharacterGlyphIndex(font_asset *Font, wchar Character)
{
    u32 GlyphIndex = 0;

    for (u32 CodepointsRangeIndex = 0; CodepointsRangeIndex < Font->CodepointsRangeCount; ++CodepointsRangeIndex)
    {
        codepoints_range *CodepointsRange = Font->CodepointsRanges + CodepointsRangeIndex;

        u32 Index = Character - CodepointsRange->Start;

        if (Index < CodepointsRange->Count)
        {
            GlyphIndex += Index;
            break;
        }
        else
        {
            GlyphIndex += CodepointsRange->Count;
        }
    }

    return GlyphIndex;
}

inline f32
GetHorizontalAdvanceForPair(font_asset *Font, wchar Character, wchar NextCharacter)
{
    f32 Result = 0.f;

    i32 RowIndex = GetCharacterGlyphIndex(Font, Character);
    i32 ColumnIndex = GetCharacterGlyphIndex(Font, NextCharacter);

    if (RowIndex >= 0 && ColumnIndex >= 0)
    {
        Assert((RowIndex * Font->GlyphCount + ColumnIndex) < (i32)Font->HorizontalAdvanceTableCount);

        Result = *(Font->HorizontalAdvanceTable + RowIndex * Font->GlyphCount + ColumnIndex);
    }

    return Result;
}

inline glyph *
GetCharacterGlyph(font_asset *Font, wchar Character)
{
    u32 GlyphIndex = GetCharacterGlyphIndex(Font, Character);
    glyph *Result = Font->Glyphs + GlyphIndex;

    return Result;
}
