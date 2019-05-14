#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "assets.h"

int main(int argc, char **argv)
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

    f32 FontHeight = 72.f;
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
    f32 FontSize = 72.f;
    s32 Start = '!';
    s32 End = '~';
    s32 Count = End - Start + 1;
    stbtt_packedchar *CharData = (stbtt_packedchar *)malloc(Count * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontBuffer, FontIndex, FontSize, Start, Count, CharData);

    stbtt_PackEnd(&PackContext);

    {
        int x0, y0, x1, y1;
        stbtt_GetFontBoundingBox(&FontInfo, &x0, &y0, &x1, &y1);

        int b = 0;
    }

    glyph_info *Glyphs = (glyph_info *)malloc(Count * sizeof(glyph_info));
    for (s32 GlyphIndex = 0; GlyphIndex < Count; ++GlyphIndex)
    {
        stbtt_packedchar *GlyphInfo = CharData + GlyphIndex;

        if (GlyphIndex == ('A' - Start))
        {
            int a = 0;
        }
        if (GlyphIndex == ('y' - Start))
        {
            int b = 0;
        }
        if (GlyphIndex == ('a' - Start))
        {
            int b = 0;
        }

        glyph_info *Glyph = Glyphs + GlyphIndex;
        *Glyph = {};
        Glyph->Size = vec2(GlyphInfo->x1 - GlyphInfo->x0, GlyphInfo->y1 - GlyphInfo->y0);
        Glyph->UV = vec2(GlyphInfo->x0, GlyphInfo->y0);
        Glyph->Alignment = vec2(GlyphInfo->xoff, GlyphInfo->yoff);
    }

    u32 HorizontalAdvanceTableCount = Count * Count;
    s32 *HorizontalAdvanceTable = (s32 *)malloc(HorizontalAdvanceTableCount * sizeof(s32));

    u32 CharacterIndex = 0;
    for(char Character = Start; Character <= End; ++Character)
    {
        s32 AdvanceWidth, LeftSideBearing;
        stbtt_GetCodepointHMetrics(&FontInfo, Character, &AdvanceWidth, &LeftSideBearing);
        AdvanceWidth = (s32)((f32)AdvanceWidth * Scale);
        LeftSideBearing = (s32)((f32)LeftSideBearing * Scale);

        u32 AnotherCharacterIndex = 0;
        for(char AnotherCharacter = Start; AnotherCharacter <= End; ++AnotherCharacter)
        {
            s32 *HorizontalAdvance = HorizontalAdvanceTable + CharacterIndex * Count + AnotherCharacterIndex;
            s32 Kerning = stbtt_GetCodepointKernAdvance(&FontInfo, Character, AnotherCharacter);
            Kerning = (s32)((f32)Kerning * Scale);
            
            if (Kerning != 0)
            {
                int b = 0;
            }

            *HorizontalAdvance = AdvanceWidth + Kerning;

            ++AnotherCharacterIndex;
        }

        ++CharacterIndex;
    }

    {
        f32 offsetX = 0;
        f32 offsetY = 0;

        u32 GlyphIndex = 0;
        for(char Character = Start; Character <= End; ++Character)
        {
            stbtt_aligned_quad quad;
            stbtt_GetPackedQuad(CharData, Width, Height, Character - Start, &offsetX, &offsetY, &quad, 1);

            glyph_info *Glyph = Glyphs + GlyphIndex;
            Glyph->x0 = quad.x0;
            Glyph->x1 = quad.x1;
            Glyph->y0 = quad.y0;
            Glyph->y1 = quad.y1;

            //Glyph->Size = vec2(quad.x1 - quad.x0, quad.y1 - quad.y0);
            Glyph->UV = vec2(quad.s0, quad.t0);
            Glyph->Alignment = vec2(0, -quad.y1);

            ++GlyphIndex;
        }
    }

    font_asset FontAsset = {};
    FontAsset.FontInfo.TextureAtlas = {};
    FontAsset.FontInfo.TextureAtlas.Width = Width;
    FontAsset.FontInfo.TextureAtlas.Height = Height;
    FontAsset.FontInfo.TextureAtlas.Channels = Channels;
    FontAsset.FontInfo.TextureAtlas.Memory = Pixels;

    FontAsset.FontInfo.VerticalAdvance = VerticalAdvance;

    FontAsset.FontInfo.HorizontalAdvanceTableCount = HorizontalAdvanceTableCount;
    FontAsset.FontInfo.HorizontalAdvanceTable = HorizontalAdvanceTable;

    FontAsset.GlyphCount = Count;
    FontAsset.Glyphs = Glyphs;

    //stbi_write_png("assets/font_atlas.png", Width, Height, Channels, Pixels, 0);
    //stbi_write_bmp("assets/font_atlas.bmp", Width, Height, Channels, Pixels);

    FILE *FontAssetFile = fopen("assets/font.fasset", "wb");

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

    FontAssetHeader.GlyphCount = Count;
    FontAssetHeader.Glyphs = FontAssetHeader.HorizontalAdvanceTable + HorizontalAdvanceTableCount * sizeof(s32);

    fwrite(&FontAssetHeader, sizeof(FontAssetHeader), 1, FontAssetFile);

    fwrite(Pixels, sizeof(u8), Width * Height * Channels, FontAssetFile);

    fwrite(HorizontalAdvanceTable, sizeof(s32), HorizontalAdvanceTableCount, FontAssetFile);

    fwrite(Glyphs, sizeof(glyph_info), Count, FontAssetFile);
    
    fclose(FontAssetFile);

    free(FontBuffer);
    free(Pixels);
    free(CharData);
    free(HorizontalAdvanceTable);
    free(Glyphs);

    //{
    //    FILE *File = fopen("assets/font.fasset", "rb");

    //    font_asset_header Header;
    //    fread(&Header, 1, sizeof(Header), File);

    //    u8 *TextureAtlas = (u8 *)malloc(Width * Height * Channels * sizeof(u8));
    //    fread(TextureAtlas, Width * Height * Channels, sizeof(u8), File);

    //    s32 *HorizontalAdvanceTable = (s32 *)malloc(HorizontalAdvanceTableCount * sizeof(s32));
    //    fread(HorizontalAdvanceTable, HorizontalAdvanceTableCount, sizeof(s32), File);

    //    glyph_info *Glyphs = (glyph_info *)malloc(Count * sizeof(glyph_info));
    //    fread(Glyphs, Count, sizeof(glyph_info), File);

    //    fclose(File);
    //    free(TextureAtlas);
    //    free(HorizontalAdvanceTable);
    //    free(Glyphs);
    //}


    return 0;
}