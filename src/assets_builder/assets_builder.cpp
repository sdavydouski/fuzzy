#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "assets.h"

int main(int ArgCount, char **Args)
{
    FILE *FontFile = fopen("c:/windows/fonts/arial.ttf", "rb");

    fseek(FontFile, 0, SEEK_END);
    u32 FileSize = ftell(FontFile);
    fseek(FontFile, 0, SEEK_SET);

    u8 *FontBuffer = (u8 *)malloc(FileSize);

    fread(FontBuffer, 1, FileSize, FontFile);

    fclose(FontFile);

    stbtt_fontinfo FontInfo;
    stbtt_InitFont(&FontInfo, FontBuffer, 0);

    f32 FontHeight = 70.f;
    f32 Scale = stbtt_ScaleForPixelHeight(&FontInfo, FontHeight);

    s32 Ascent, Descent, LineGap;
    stbtt_GetFontVMetrics(&FontInfo, &Ascent, &Descent, &LineGap);
    Ascent = (s32)((f32)Ascent * Scale);
    Descent = (s32)((f32)Descent * Scale);
    LineGap = (s32)((f32)LineGap * Scale);

    s32 VerticalAdvance = Ascent - Descent + LineGap;

    stbtt_pack_context PackContext;
    s32 TextureWidth = 1024;
    s32 TextureHeight = 1024;
    s32 TextureChannels = 1;
    u8 *Pixels = (u8 *)malloc(TextureWidth * TextureHeight * TextureChannels);
    s32 StrideInBytes = 0;
    s32 Padding = 1;

    stbtt_PackBegin(&PackContext, Pixels, TextureWidth, TextureHeight, StrideInBytes, Padding, NULL);

    u32 hOverSample = 2;
    u32 vOverSample = 2;
    stbtt_PackSetOversampling(&PackContext, hOverSample, vOverSample);

    s32 FontIndex = 0;

    u32 CodepointsRangeCount = 2;
    codepoints_range *CodepointsRanges = (codepoints_range *)malloc(CodepointsRangeCount * sizeof(codepoints_range));

    codepoints_range *Latin = CodepointsRanges + 0;
    Latin->Start = ' ';
    Latin->End = '~';
    Latin->Count = GetCodepointsRangeCount(Latin);

    stbtt_packedchar *LatinCharData = (stbtt_packedchar *)malloc(Latin->Count * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontBuffer, FontIndex, FontHeight, Latin->Start, Latin->Count, LatinCharData);

    codepoints_range *Cyrillic = CodepointsRanges + 1;
    Cyrillic->Start = 0x410;
    Cyrillic->End = 0x44F;
    Cyrillic->Count = GetCodepointsRangeCount(Cyrillic);

    stbtt_packedchar *CyrillicCharData = (stbtt_packedchar *)malloc(Cyrillic->Count * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontBuffer, FontIndex, FontHeight, Cyrillic->Start, Cyrillic->Count, CyrillicCharData);

    u32 TotalCodepointCount = Latin->Count + Cyrillic->Count;

    stbtt_PackEnd(&PackContext);

    glyph_info *Glyphs = (glyph_info *)malloc(TotalCodepointCount * sizeof(glyph_info));

    for(u32 GlyphIndex = 0; GlyphIndex < TotalCodepointCount; ++GlyphIndex)
    {
        glyph_info *Glyph = Glyphs + GlyphIndex;

        stbtt_packedchar *GlyphInfo = nullptr;

        // todo:
        if (GlyphIndex < Latin->Count)
        {
            u32 Index = GlyphIndex;
            GlyphInfo = LatinCharData + Index;
        }
        else if (Latin->Count <= GlyphIndex && GlyphIndex < Latin->Count + Cyrillic->Count)
        {
            u32 Index = GlyphIndex - Latin->Count;
            GlyphInfo = CyrillicCharData + Index;
        }
        else
        {
            InvalidCodePath;
        }

        Glyph->SpriteSize = vec2(GlyphInfo->x1 - GlyphInfo->x0, GlyphInfo->y1 - GlyphInfo->y0);
    }

    u32 HorizontalAdvanceTableCount = TotalCodepointCount * TotalCodepointCount;
    f32 *HorizontalAdvanceTable = (f32 *)malloc(HorizontalAdvanceTableCount * sizeof(f32));

    u32 CodepointIndex = 0;
    while (CodepointIndex < TotalCodepointCount)
    {
        stbtt_packedchar *CharData = nullptr;
        codepoints_range *Encoding = nullptr;
        s32 CharacterIndex;

        // todo:
        if (CodepointIndex < Latin->Count)
        {
            Encoding = Latin;
            CharData = LatinCharData;
            CharacterIndex = CodepointIndex;
        }
        else if (Latin->Count <= CodepointIndex && CodepointIndex < Latin->Count + Cyrillic->Count)
        {
            Encoding = Cyrillic;
            CharData = CyrillicCharData;
            CharacterIndex = CodepointIndex - Latin->Count;
        }
        else
        {
            InvalidCodePath;
        }

        wchar_t Character = Encoding->Start + CharacterIndex;

        f32 OffsetX = 0;
        f32 OffsetY = 0;
        stbtt_aligned_quad Quad;
        stbtt_GetPackedQuad(CharData, TextureWidth, TextureHeight, Character - Encoding->Start, &OffsetX, &OffsetY, &Quad, 1);

        glyph_info *Glyph = Glyphs + CodepointIndex;

        Glyph->CharacterSize = vec2(Quad.x1 - Quad.x0, Quad.y1 - Quad.y0);
        Glyph->UV = vec2(Quad.s0, Quad.t0);
        Glyph->Alignment = vec2(Quad.x0, -Quad.y1);

        u32 OtherCodepointIndex = 0;
        while (OtherCodepointIndex < TotalCodepointCount)
        {
            codepoints_range *Encoding = nullptr;
            s32 OtherCharacterIndex;

            // todo:
            if (OtherCodepointIndex < Latin->Count)
            {
                Encoding = Latin;
                OtherCharacterIndex = OtherCodepointIndex;
            }
            else if (Latin->Count <= OtherCodepointIndex && OtherCodepointIndex < Latin->Count + Cyrillic->Count)
            {
                Encoding = Cyrillic;
                OtherCharacterIndex = OtherCodepointIndex - Latin->Count;
            }
            else
            {
                InvalidCodePath;
            }

            wchar_t OtherCharacter = Encoding->Start + OtherCharacterIndex;

            f32 *HorizontalAdvance = HorizontalAdvanceTable + CodepointIndex * TotalCodepointCount + OtherCodepointIndex;
            f32 Kerning = (f32)stbtt_GetCodepointKernAdvance(&FontInfo, Character, OtherCharacter);
            Kerning = Kerning * Scale;

            *HorizontalAdvance = OffsetX + Kerning;

            ++OtherCodepointIndex;
        }

        ++CodepointIndex;
    }

    // just for testing
    stbi_write_png("assets/font_atlas.png", TextureWidth, TextureHeight, TextureChannels, Pixels, 0);

    FILE *FontAssetFile = fopen("assets/font.fasset", "wb");

    font_asset_header FontAssetHeader = {};
    FontAssetHeader.MagicValue = 0x451;
    FontAssetHeader.Version = 1;

    u32 PixelCount = TextureWidth * TextureHeight * TextureChannels;

    FontAssetHeader.TextureAtlasWidth = TextureWidth;
    FontAssetHeader.TextureAtlasHeight = TextureHeight;
    FontAssetHeader.TextureAtlasChannels = TextureChannels;
    FontAssetHeader.TextureAtlas = sizeof(FontAssetHeader);

    FontAssetHeader.VerticalAdvance = VerticalAdvance;
    //FontAssetHeader.Ascent = Ascent;
    //FontAssetHeader.Descent = Descent;

    FontAssetHeader.HorizontalAdvanceTableCount = HorizontalAdvanceTableCount;
    FontAssetHeader.HorizontalAdvanceTable = FontAssetHeader.TextureAtlas + PixelCount * sizeof(u8);

    FontAssetHeader.GlyphCount = TotalCodepointCount;
    FontAssetHeader.Glyphs = FontAssetHeader.HorizontalAdvanceTable + HorizontalAdvanceTableCount * sizeof(s32);

    FontAssetHeader.CodepointsRangeCount = CodepointsRangeCount;
    FontAssetHeader.CodepointsRanges = FontAssetHeader.Glyphs + TotalCodepointCount * sizeof(glyph_info);

    fwrite(&FontAssetHeader, sizeof(FontAssetHeader), 1, FontAssetFile);
    fwrite(Pixels, sizeof(u8), PixelCount, FontAssetFile);
    fwrite(HorizontalAdvanceTable, sizeof(f32), HorizontalAdvanceTableCount, FontAssetFile);
    fwrite(Glyphs, sizeof(glyph_info), TotalCodepointCount, FontAssetFile);
    fwrite(CodepointsRanges, sizeof(codepoints_range), CodepointsRangeCount, FontAssetFile);
    
    fclose(FontAssetFile);

    free(FontBuffer);
    free(Pixels);
    free(LatinCharData);
    free(CyrillicCharData);
    free(HorizontalAdvanceTable);
    free(Glyphs);
    free(CodepointsRanges);

    return 0;
}