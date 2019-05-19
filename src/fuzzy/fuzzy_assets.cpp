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
    FILE *File = fopen("assets/font.fasset", "rb");

    font_asset_header Header;
    fread(&Header, 1, sizeof(Header), File);

    u8 *TextureAtlas = (u8 *)malloc(Header.TextureAtlasWidth * Header.TextureAtlasHeight * Header.TextureAtlasChannels * sizeof(u8));
    fread(TextureAtlas, Header.TextureAtlasWidth * Header.TextureAtlasHeight * Header.TextureAtlasChannels, sizeof(u8), File);

    f32 *HorizontalAdvanceTable = (f32 *)malloc(Header.HorizontalAdvanceTableCount * sizeof(f32));
    fread(HorizontalAdvanceTable, Header.HorizontalAdvanceTableCount, sizeof(f32), File);

    glyph_info *Glyphs = (glyph_info *)malloc(Header.GlyphCount * sizeof(glyph_info));
    fread(Glyphs, Header.GlyphCount, sizeof(glyph_info), File);

    codepoints_range *CodepointsRanges = (codepoints_range *)malloc(Header.CodepointsRangeCount * sizeof(codepoints_range));
    fread(CodepointsRanges, Header.CodepointsRangeCount, sizeof(codepoints_range), File);

    fclose(File);
    //free(TextureAtlas);
    //free(HorizontalAdvanceTable);
    //free(Glyphs);

    GameState->Assets = {};
    GameState->Assets.FontInfo.TextureAtlas = {};
    GameState->Assets.FontInfo.TextureAtlas.Width = Header.TextureAtlasWidth;
    GameState->Assets.FontInfo.TextureAtlas.Height = Header.TextureAtlasHeight;
    GameState->Assets.FontInfo.TextureAtlas.Channels = Header.TextureAtlasChannels;
    GameState->Assets.FontInfo.TextureAtlas.Memory = TextureAtlas;

    GameState->Assets.FontInfo.VerticalAdvance = Header.VerticalAdvance;
    //GameState->Assets.FontInfo.Ascent = Header.Ascent;
    //GameState->Assets.FontInfo.Descent = Header.Descent;

    GameState->Assets.FontInfo.HorizontalAdvanceTableCount = Header.HorizontalAdvanceTableCount;
    GameState->Assets.FontInfo.HorizontalAdvanceTable = HorizontalAdvanceTable;

    GameState->Assets.GlyphCount = Header.GlyphCount;
    GameState->Assets.Glyphs = Glyphs;

    GameState->Assets.FontInfo.CodepointsRangeCount = Header.CodepointsRangeCount;
    GameState->Assets.FontInfo.CodepointsRanges = CodepointsRanges;
#endif
}