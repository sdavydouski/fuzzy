#include "fuzzy_types.h"
#include "fuzzy_platform.h"
#include "fuzzy.h"

internal_function void
LoadGameAssets(platform_api *Platform, game_state *GameState)
{
#if 0
    read_file_result AssetsFile = Platform->ReadFile("assets/font.fasset");

    // todo: implement without STL
#else
    FILE *File = fopen("assets/data.fasset", "rb");

    asset_header AssetHeader;
    fread(&AssetHeader, 1, sizeof(asset_header), File);

    GameState->FontAssetCount = AssetHeader.FontCount;
    GameState->FontAssets = (font_asset *)malloc(AssetHeader.FontCount * sizeof(font_asset));

    for (u32 FontAssetIndex = 0; FontAssetIndex < AssetHeader.FontCount; ++FontAssetIndex) {
        font_asset *FontAsset = GameState->FontAssets + FontAssetIndex;

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

        FontAsset->TextureAtlas.Width = FontAssetHeader.TextureAtlasWidth;
        FontAsset->TextureAtlas.Height = FontAssetHeader.TextureAtlasHeight;
        FontAsset->TextureAtlas.Channels = FontAssetHeader.TextureAtlasChannels;
        FontAsset->TextureAtlas.Memory = TextureAtlas;

        FontAsset->VerticalAdvance = FontAssetHeader.VerticalAdvance;
        FontAsset->Ascent = FontAssetHeader.Ascent;
        FontAsset->Descent = FontAssetHeader.Descent;

        FontAsset->CodepointsRangeCount = FontAssetHeader.CodepointsRangeCount;
        FontAsset->CodepointsRanges = CodepointsRanges;

        FontAsset->HorizontalAdvanceTableCount = FontAssetHeader.HorizontalAdvanceTableCount;
        FontAsset->HorizontalAdvanceTable = HorizontalAdvanceTable;

        FontAsset->GlyphCount = FontAssetHeader.GlyphCount;
        FontAsset->Glyphs = Glyphs;
    }

    fclose(File);
#endif
}