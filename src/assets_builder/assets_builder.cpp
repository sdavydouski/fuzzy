#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "fuzzy_types.h"

int main(int argc, char **argv)
{
    FILE *FontFile = fopen("c:/windows/fonts/arial.ttf", "rb");

    fseek(FontFile, 0, SEEK_END);
    u32 FileSize = ftell(FontFile);
    fseek(FontFile, 0, SEEK_SET);

    u8 *FontData = (u8 *)malloc(FileSize);

    fread(FontData, 1, FileSize, FontFile);
    fclose(FontFile);

    stbtt_pack_context PackContext;

    s32 Width = 1024;
    s32 Height = 1024;
    u8 *Pixels = (u8 *)malloc(Width * Height);
    s32 StrideInBytes = 0;
    s32 Padding = 1;

    stbtt_PackBegin(&PackContext, Pixels, Width, Height, StrideInBytes, Padding, NULL);

    u32 hOverSample = 2;
    u32 vOverSample = 2;
    stbtt_PackSetOversampling(&PackContext, hOverSample, vOverSample);

    s32 FontIndex = 0;
    f32 FontSize = 72.f;
    s32 StartFrom = '!';
    s32 Count = '~' - '!' + 1;
    stbtt_packedchar *CharData = (stbtt_packedchar *)malloc(Count * sizeof(stbtt_packedchar));
    stbtt_PackFontRange(&PackContext, FontData, FontIndex, FontSize, StartFrom, Count, CharData);

    stbtt_PackEnd(&PackContext);

    stbi_write_png("assets/font_atlas.png", Width, Height, 1, Pixels, 0);

    FILE *FontMetricsFile = fopen("assets/font_metrics.azaza", "wb");
    fwrite(CharData, sizeof(stbtt_packedchar), Count, FontMetricsFile);
    fclose(FontMetricsFile);

    free(FontData);
    free(Pixels);
    free(CharData);

    return 0;
}