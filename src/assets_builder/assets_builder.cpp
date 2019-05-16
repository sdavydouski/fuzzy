#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "assets.h"

constexpr auto ASSET_FILE_NAME = "assets/font.fasset";

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
    s32 Width = 1024;
    s32 Height = 1024;
    s32 Channels = 1;
    u8 *Pixels = (u8 *)malloc(Width * Height * Channels);
    s32 StrideInBytes = 0;
    s32 Padding = 1;

    stbtt_PackBegin(&PackContext, Pixels, Width, Height, StrideInBytes, Padding, NULL);

    u32 hOverSample = 2;
    u32 vOverSample = 2;
    stbtt_PackSetOversampling(&PackContext, hOverSample, vOverSample);

    s32 FontIndex = 0;
    f32 FontSize = FontHeight;

    font_range English = {};
    English.CodepointStart = ' ';
    English.CodepointEnd = '~';
    s32 EnglishCount = GetFontRangeCount(&English);

    font_range Russian = {};
    Russian.CodepointStart = 0x410;
    Russian.CodepointEnd = 0x44F;
    s32 RussianCount = GetFontRangeCount(&Russian);

    s32 TotalCount = EnglishCount + RussianCount;

    stbtt_packedchar *EnglishCharData = (stbtt_packedchar *)malloc(EnglishCount * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontBuffer, FontIndex, FontSize, English.CodepointStart, EnglishCount, EnglishCharData);

    stbtt_packedchar *RussianCharData = (stbtt_packedchar *)malloc(RussianCount * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontBuffer, FontIndex, FontSize, Russian.CodepointStart, RussianCount, RussianCharData);

    stbtt_PackEnd(&PackContext);

    glyph_info *EnglishGlyphs = (glyph_info *)malloc(EnglishCount * sizeof(glyph_info));
    for(s32 GlyphIndex = 0; GlyphIndex < EnglishCount; ++GlyphIndex)
    {
        stbtt_packedchar *GlyphInfo = EnglishCharData + GlyphIndex;

        glyph_info *Glyph = EnglishGlyphs + GlyphIndex;
        *Glyph = {};
        Glyph->SpriteSize = vec2(GlyphInfo->x1 - GlyphInfo->x0, GlyphInfo->y1 - GlyphInfo->y0);
    }

    glyph_info *RussianGlyphs = (glyph_info *)malloc(RussianCount * sizeof(glyph_info));
    for(s32 GlyphIndex = 0; GlyphIndex < RussianCount; ++GlyphIndex)
    {
        stbtt_packedchar *GlyphInfo = RussianCharData + GlyphIndex;

        glyph_info *Glyph = RussianGlyphs + GlyphIndex;
        *Glyph = {};
        Glyph->SpriteSize = vec2(GlyphInfo->x1 - GlyphInfo->x0, GlyphInfo->y1 - GlyphInfo->y0);
    }

    u32 HorizontalAdvanceTableCount = EnglishCount * EnglishCount;
    f32 *HorizontalAdvanceTable = (f32 *)malloc(HorizontalAdvanceTableCount * sizeof(f32));

    u32 CharacterIndex = 0;
    for(char Character = English.CodepointStart; Character <= English.CodepointEnd; ++Character)
    {
        f32 OffsetX = 0;
        f32 OffsetY = 0;
        stbtt_aligned_quad Quad;
        stbtt_GetPackedQuad(EnglishCharData, Width, Height, Character - English.CodepointStart, &OffsetX, &OffsetY, &Quad, 1);

        glyph_info *Glyph = EnglishGlyphs + CharacterIndex;

        Glyph->CharacterSize = vec2(Quad.x1 - Quad.x0, Quad.y1 - Quad.y0);
        Glyph->UV = vec2(Quad.s0, Quad.t0);
        Glyph->Alignment = vec2(Quad.x0, -Quad.y1);

        u32 OtherCharacterIndex = 0;
        for(char OtherCharacter = English.CodepointStart; OtherCharacter <= English.CodepointEnd; ++OtherCharacter)
        {
            f32 *HorizontalAdvance = HorizontalAdvanceTable + CharacterIndex * EnglishCount + OtherCharacterIndex;
            f32 Kerning = (f32)stbtt_GetCodepointKernAdvance(&FontInfo, Character, OtherCharacter);
            Kerning = Kerning * Scale;

            *HorizontalAdvance = OffsetX + Kerning;

            ++OtherCharacterIndex;
        }

        ++CharacterIndex;
    }

    // just for testing
    stbi_write_png("assets/font_atlas.png", Width, Height, Channels, Pixels, 0);

    FILE *FontAssetFile = fopen(ASSET_FILE_NAME, "wb");

    font_asset_header FontAssetHeader = {};
    FontAssetHeader.MagicValue = 0x451;
    FontAssetHeader.Version = 1;

    FontAssetHeader.TextureAtlasWidth = Width;
    FontAssetHeader.TextureAtlasHeight = Height;
    FontAssetHeader.TextureAtlasChannels = Channels;
    FontAssetHeader.TextureAtlas = sizeof(FontAssetHeader);

    FontAssetHeader.VerticalAdvance = VerticalAdvance;
    FontAssetHeader.Ascent = Ascent;
    FontAssetHeader.Descent = Descent;

    FontAssetHeader.HorizontalAdvanceTableCount = HorizontalAdvanceTableCount;
    FontAssetHeader.HorizontalAdvanceTable = FontAssetHeader.TextureAtlas + Width * Height * Channels * sizeof(u8);

    FontAssetHeader.GlyphCount = EnglishCount;
    FontAssetHeader.Glyphs = FontAssetHeader.HorizontalAdvanceTable + HorizontalAdvanceTableCount * sizeof(s32);

    fwrite(&FontAssetHeader, sizeof(FontAssetHeader), 1, FontAssetFile);
    fwrite(Pixels, sizeof(u8), Width * Height * Channels, FontAssetFile);
    fwrite(HorizontalAdvanceTable, sizeof(f32), HorizontalAdvanceTableCount, FontAssetFile);
    fwrite(EnglishGlyphs, sizeof(glyph_info), EnglishCount, FontAssetFile);
    
    fclose(FontAssetFile);

    free(FontBuffer);
    free(Pixels);
    free(EnglishCharData);
    free(RussianCharData);
    free(HorizontalAdvanceTable);
    free(EnglishGlyphs);
    free(RussianGlyphs);

    return 0;
}