﻿#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "assets.h"

void
PrepareFontAsset(
    char *FontFileName,
    f32 FontHeight,
    char *FontTextureAtlasFileName, 
    font_asset *FontAsset
)
{
    FILE *FontFile = fopen(FontFileName, "rb");

    fseek(FontFile, 0, SEEK_END);
    u32 FileSize = ftell(FontFile);
    fseek(FontFile, 0, SEEK_SET);

    u8 *FontBuffer = (u8 *)malloc(FileSize);

    fread(FontBuffer, 1, FileSize, FontFile);

    fclose(FontFile);

    stbtt_fontinfo FontInfo;
    stbtt_InitFont(&FontInfo, FontBuffer, 0);

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

    glyph *Glyphs = (glyph *)malloc(TotalCodepointCount * sizeof(glyph));

    for(u32 GlyphIndex = 0; GlyphIndex < TotalCodepointCount; ++GlyphIndex)
    {
        glyph *Glyph = Glyphs + GlyphIndex;

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

        wchar Character = Encoding->Start + CharacterIndex;

        f32 OffsetX = 0;
        f32 OffsetY = 0;
        stbtt_aligned_quad Quad;
        stbtt_GetPackedQuad(CharData, TextureWidth, TextureHeight, Character - Encoding->Start, &OffsetX, &OffsetY, &Quad, 1);

        glyph *Glyph = Glyphs + CodepointIndex;

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

            wchar OtherCharacter = Encoding->Start + OtherCharacterIndex;

            f32 *HorizontalAdvance = HorizontalAdvanceTable + CodepointIndex * TotalCodepointCount + OtherCodepointIndex;
            f32 Kerning = (f32)stbtt_GetCodepointKernAdvance(&FontInfo, Character, OtherCharacter);
            Kerning = Kerning * Scale;

            *HorizontalAdvance = OffsetX + Kerning;

            ++OtherCodepointIndex;
        }

        ++CodepointIndex;
    }

    // for testing
    stbi_write_png(FontTextureAtlasFileName, TextureWidth, TextureHeight, TextureChannels, Pixels, 0);

    //FILE *FontAssetFile = fopen(FontAssetFileName, "wb");

    //FontAsset->Name = FontName;
    
    FontAsset->TextureAtlas.Width = TextureWidth;
    FontAsset->TextureAtlas.Height = TextureHeight;
    FontAsset->TextureAtlas.Channels = TextureChannels;
    FontAsset->TextureAtlas.Memory = Pixels;

    FontAsset->VerticalAdvance = VerticalAdvance;
    FontAsset->Ascent = Ascent;
    FontAsset->Descent = Descent;

    FontAsset->CodepointsRangeCount = CodepointsRangeCount;
    FontAsset->CodepointsRanges = CodepointsRanges;

    FontAsset->HorizontalAdvanceTableCount = HorizontalAdvanceTableCount;
    FontAsset->HorizontalAdvanceTable = HorizontalAdvanceTable;

    FontAsset->GlyphCount = TotalCodepointCount;
    FontAsset->Glyphs = Glyphs;

    
    free(FontBuffer);
    //free(Pixels);
    //free(LatinCharData);
    //free(CyrillicCharData);
    //free(HorizontalAdvanceTable);
    //free(Glyphs);
    //free(CodepointsRanges);
}

// todo: check for memory leaks (do i care?)
void
PackFonts()
{
    u32 FontAssetCount = 2;
    font_asset *FontAssets = (font_asset *)malloc(FontAssetCount * sizeof(font_asset));

    PrepareFontAsset((char *)"c:/windows/fonts/arial.ttf", 70.f, (char *)"assets/font_arial.png", FontAssets + 0);
    PrepareFontAsset((char *)"c:/windows/fonts/lucon.ttf", 60.f, (char *)"assets/font_lucida.png", FontAssets + 1);

    asset_header Header = {};
    Header.MagicValue = 0x451;
    Header.Version = 1;

    Header.FontCount = FontAssetCount;
    Header.FontsOffset = sizeof(asset_header);

    FILE *AssetFile = fopen("assets/data.fasset", "wb");

    fwrite(&Header, sizeof(asset_header), 1, AssetFile);

    for (u32 FontAssetIndex = 0; FontAssetIndex < FontAssetCount; ++FontAssetIndex) {
        font_asset *FontAsset = FontAssets + FontAssetIndex;

        font_asset_header FontAssetHeader = {};

        s32 PixelCount = FontAsset->TextureAtlas.Width * FontAsset->TextureAtlas.Height * FontAsset->TextureAtlas.Channels;

        FontAssetHeader.TextureAtlasWidth = FontAsset->TextureAtlas.Width;
        FontAssetHeader.TextureAtlasHeight = FontAsset->TextureAtlas.Height;
        FontAssetHeader.TextureAtlasChannels = FontAsset->TextureAtlas.Channels;
        FontAssetHeader.TextureAtlasOffset = Header.FontsOffset + FontAssetIndex * sizeof(font_asset_header);

        FontAssetHeader.VerticalAdvance = FontAsset->VerticalAdvance;
        FontAssetHeader.Ascent = FontAsset->Ascent;
        FontAssetHeader.Descent = FontAsset->Descent;

        FontAssetHeader.CodepointsRangeCount = FontAsset->CodepointsRangeCount;
        FontAssetHeader.CodepointsRangesOffset = FontAssetHeader.TextureAtlasOffset + PixelCount * sizeof(u8);

        FontAssetHeader.HorizontalAdvanceTableCount = FontAsset->HorizontalAdvanceTableCount;
        FontAssetHeader.HorizontalAdvanceTableOffset = FontAssetHeader.CodepointsRangesOffset + FontAsset->CodepointsRangeCount * sizeof(codepoints_range);

        FontAssetHeader.GlyphCount = FontAsset->GlyphCount;
        FontAssetHeader.GlyphsOffset = FontAssetHeader.HorizontalAdvanceTableOffset + FontAsset->HorizontalAdvanceTableCount * sizeof(f32);

        fwrite(&FontAssetHeader, sizeof(FontAssetHeader), 1, AssetFile);
        fwrite(FontAsset->TextureAtlas.Memory, sizeof(u8), PixelCount, AssetFile);
        fwrite(FontAsset->HorizontalAdvanceTable, sizeof(f32), FontAsset->HorizontalAdvanceTableCount, AssetFile);
        fwrite(FontAsset->CodepointsRanges, sizeof(codepoints_range), FontAsset->CodepointsRangeCount, AssetFile);
        fwrite(FontAsset->Glyphs, sizeof(glyph), FontAsset->GlyphCount, AssetFile);
    }

    fclose(AssetFile);
    free(FontAssets);

#if 0
    {
        FILE *File = fopen("assets/data.fasset", "rb");

        asset_header AssetHeader;
        fread(&AssetHeader, 1, sizeof(asset_header), File);

        for (u32 FontAssetIndex = 0; FontAssetIndex < AssetHeader.FontCount; ++FontAssetIndex) {
            font_asset_header FontAssetHeader;
            fread(&FontAssetHeader, 1, sizeof(font_asset_header), File);

            u8 *TextureAtlas = (u8 *)malloc(FontAssetHeader.TextureAtlasWidth * FontAssetHeader.TextureAtlasHeight * FontAssetHeader.TextureAtlasChannels * sizeof(u8));
            fread(TextureAtlas, FontAssetHeader.TextureAtlasWidth * FontAssetHeader.TextureAtlasHeight * FontAssetHeader.TextureAtlasChannels, sizeof(u8), File);

            f32 *HorizontalAdvanceTable = (f32 *)malloc(FontAssetHeader.HorizontalAdvanceTableCount * sizeof(f32));
            fread(HorizontalAdvanceTable, FontAssetHeader.HorizontalAdvanceTableCount, sizeof(f32), File);

            codepoints_range *CodepointsRanges = (codepoints_range *)malloc(FontAssetHeader.CodepointsRangeCount * sizeof(codepoints_range));
            fread(CodepointsRanges, FontAssetHeader.CodepointsRangeCount, sizeof(codepoints_range), File);

            glyph *Glyphs = (glyph *)malloc(FontAssetHeader.GlyphCount * sizeof(glyph));
            fread(Glyphs, FontAssetHeader.GlyphCount, sizeof(glyph), File);
        }

        fclose(File);
    }
#endif
}

int main(int ArgCount, char **Args)
{
    PackFonts();

    return 0;
}