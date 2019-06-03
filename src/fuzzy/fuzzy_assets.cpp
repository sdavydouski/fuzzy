#include "fuzzy_types.h"
#include "fuzzy_platform.h"
#include "fuzzy.h"

internal_function void
LoadGameAssets(platform_api *Platform, game_state *GameState, memory_arena *Arena)
{
    read_file_result AssetFile = Platform->ReadFile("assets/data.fasset");

    asset_header *AssetHeader = (asset_header *)AssetFile.Contents;

    assert(AssetHeader->MagicValue == 0x451);

    GameState->FontAssetCount = AssetHeader->FontCount;
    GameState->FontAssets = PushArray<font_asset>(Arena, GameState->FontAssetCount);

    u64 FontAssetHeaderOffset = AssetHeader->FontsOffset;
    for (u32 FontAssetIndex = 0; FontAssetIndex < GameState->FontAssetCount; ++FontAssetIndex)
    {
        font_asset *FontAsset = GameState->FontAssets + FontAssetIndex;

        font_asset_header *FontAssetHeader = (font_asset_header *)((u8 *)AssetFile.Contents + FontAssetHeaderOffset);

        u32 PixelCount = FontAssetHeader->TextureAtlasWidth * FontAssetHeader->TextureAtlasHeight * FontAssetHeader->TextureAtlasChannels;
        u8 *TextureAtlas = PushArray<u8>(Arena, PixelCount);
        CopyMemoryBlock(
            (u8 *)AssetFile.Contents + FontAssetHeader->TextureAtlasOffset, 
            TextureAtlas, PixelCount * sizeof(u8)
        );

        codepoints_range *CodepointsRanges = PushArray<codepoints_range>(Arena, FontAssetHeader->CodepointsRangeCount);
        CopyMemoryBlock(
            (u8 *)AssetFile.Contents + FontAssetHeader->CodepointsRangesOffset, 
            CodepointsRanges, FontAssetHeader->CodepointsRangeCount * sizeof(codepoints_range)
        );

        f32 *HorizontalAdvanceTable = PushArray<f32>(Arena, FontAssetHeader->HorizontalAdvanceTableCount);
        CopyMemoryBlock(
            (u8 *)AssetFile.Contents + FontAssetHeader->HorizontalAdvanceTableOffset, 
            HorizontalAdvanceTable, FontAssetHeader->HorizontalAdvanceTableCount * sizeof(f32)
        );

        glyph *Glyphs = PushArray<glyph>(Arena, FontAssetHeader->GlyphCount);
        CopyMemoryBlock(
            (u8 *)AssetFile.Contents + FontAssetHeader->GlyphsOffset, 
            Glyphs, FontAssetHeader->GlyphCount * sizeof(glyph)
        );

        FontAssetHeaderOffset += (FontAssetIndex + 1) * (
            sizeof(font_asset_header) + 
            PixelCount * sizeof(u8) + FontAssetHeader->CodepointsRangeCount * sizeof(codepoints_range) +
            FontAssetHeader->HorizontalAdvanceTableCount * sizeof(f32) + FontAssetHeader->GlyphCount * sizeof(glyph)
        );

        FontAsset->TextureAtlas.Width = FontAssetHeader->TextureAtlasWidth;
        FontAsset->TextureAtlas.Height = FontAssetHeader->TextureAtlasHeight;
        FontAsset->TextureAtlas.Channels = FontAssetHeader->TextureAtlasChannels;
        FontAsset->TextureAtlas.Memory = TextureAtlas;

        FontAsset->VerticalAdvance = FontAssetHeader->VerticalAdvance;
        FontAsset->Ascent = FontAssetHeader->Ascent;
        FontAsset->Descent = FontAssetHeader->Descent;

        FontAsset->CodepointsRangeCount = FontAssetHeader->CodepointsRangeCount;
        FontAsset->CodepointsRanges = CodepointsRanges;

        FontAsset->HorizontalAdvanceTableCount = FontAssetHeader->HorizontalAdvanceTableCount;
        FontAsset->HorizontalAdvanceTable = HorizontalAdvanceTable;

        FontAsset->GlyphCount = FontAssetHeader->GlyphCount;
        FontAsset->Glyphs = Glyphs;
    }

    Platform->FreeFile(AssetFile);
}