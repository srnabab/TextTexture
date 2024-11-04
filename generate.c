#include <ft2build.h>
#include <freetype/freetype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cglm/cglm.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistr.h>

#define CHANNELS 1
#define MAX_2D_SIZW 16383

#define HASH_SIZE 6000

typedef struct _HASH
{
    vec2 uv[4];
    unistring_uint32_t utf32;
} Hash;

Hash hash[HASH_SIZE];

static void create_hash(void)
{
    for (int i = 0;i < HASH_SIZE;i++)
    {
        hash[i].utf32 = 0x110000;
    }
}

static uint32_t hash_func1(unistring_uint32_t utf32)
{
    return utf32 % HASH_SIZE;
}

static uint32_t hash_func2(unistring_uint32_t utf32)
{
    return 1 + utf32 % 5993/*(HASH_SIZE - 1)*/;
}

static bool insert(unistring_uint32_t utf32)
{
    uint32_t n = hash_func1(utf32);

    if (hash[n].utf32 == 0x110000)
    {
        hash[n].utf32 = utf32;
    }
    else
    {
        int i = 1;
        uint32_t step = hash_func2(utf32);
        uint32_t index = (n + step) % HASH_SIZE;
        while (i <= 6000)
        {
            if (hash[index].utf32 == 0x110000)
            {
                hash[index].utf32 = utf32;
            }
            i++;
            index = (n + i * step) % HASH_SIZE;
        }
        if (i == 6001)
        {
            return false;
        }
        hash[index].utf32 = utf32;
    }

    return true;
}

static int find_value(unistring_uint32_t utf32)
{
    uint32_t n = hash_func1(utf32);

    if (hash[n].utf32 == 0x110000)
    {
        return -1;
    }
    else if (hash[n].utf32 == utf32)
    {
        return n;
    }
    else
    {
        int i = 1;
        uint32_t step = hash_func2(utf32);
        uint32_t index = (n + step) % HASH_SIZE;
        while (i <= 6000)
        {
            if (hash[index].utf32 == utf32)
                return index;
            i++;
            index = (n + i * step) % HASH_SIZE;
        }

        return -1;
    }
}

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        printf(".exe [-F] [FontPath] [-T] [textPath] [-H] [hashTablePath] [-S] [fontSize]\n");
        return -1;
    }

    char fontPath[100];
    char textPath[100];
    char hashPath[100];
    int fontSize = 60;
    for (int i = 1;i < argc;i++)
    {
        if (strcmp(argv[i], "-F") == 0)
        {
            strcpy(fontPath, argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-T") == 0)
        {
            strcpy(textPath, argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-H") == 0)
        {
            strcpy(hashPath, argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-S") == 0)
        {
            fontSize = atoi(argv[i + 1]);
            i++;
        }
    }

    create_hash();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) 
    {
        fprintf(stderr, "Could not init FreeType Library\n");
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) 
    {
        fprintf(stderr, "Failed to load font\n");
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    FILE * fp;
    if ((fp = fopen(textPath, "rb")) == NULL)
    {
        printf("open text file failed\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    printf("%ld\n", size);

    unsigned char text[size];

    fread(text, sizeof(unsigned char), size, fp);
    fclose(fp);

    if (u8_check(text, size) < 0) 
    {
        printf("Input text is not valid UTF-8\n");
        return -1;
    }

    unistring_uint32_t * string;
    size_t utf32Len;

    unistring_uint32_t * utf32Result = u8_to_u32(text, size, string, &utf32Len);
    if (string == NULL)
    {
        printf("convert failed\n");
        return -1;
    }
    int count = 0;
    uint16_t index[utf32Len];

    for (int i = 0;i < utf32Len;i++)
    {
        if (find_value(utf32Result[i]) >= 0)
            continue;
        else
        {
            insert(utf32Result[i]);
            index[count] = i;
            count++;
        }
    }
    //printf("count: %d\n", count);

    //SBFile sbfile = {}; 
    //SBF_SaveNewFile("hash.sbf", (void *)hash, &len, 1, LITTLE_ENDIAN);
    const int characterPerLine = MAX_2D_SIZW / fontSize;

    const int characterCount = count;
    const int line = characterCount / characterPerLine + 1;

    int imageWidth = fontSize;
    int pixelsPerLine = imageWidth * CHANNELS;
    int imageHeight = fontSize;
    unsigned char * buffer = (unsigned char*)calloc(imageWidth * imageHeight * line * characterPerLine, sizeof(unsigned char));

    float yOffset = 1 / (float)line;
    float xOffset = 1 / (float)characterPerLine;

    FT_ULong unicode_char[characterCount];
    for (int i = 0;i < characterCount;i++)
    {
        FT_ULong utf32 = utf32Result[index[i]];
        unicode_char[i] = utf32;
        int n = find_value(utf32);
        
        static int yStep = 0;
        static int xStep = 0;
        float xOffsets = xStep * xOffset;
        float yOffsets = yStep * yOffset;
        hash[n].uv[0][0] = xOffsets;
        hash[n].uv[0][1] = yOffsets + yOffset;
        hash[n].uv[1][0] = xOffsets + xOffset;
        hash[n].uv[1][1] = yOffsets + yOffset;
        hash[n].uv[2][0] = xOffsets + xOffset;
        hash[n].uv[2][1] = yOffsets;
        hash[n].uv[3][0] = xOffsets;
        hash[n].uv[3][1] = yOffsets;
        
        xStep = (xStep + 1) % characterPerLine;
        yStep = i / characterPerLine;

        /*for (int x = 0;x < 4;x++)
        {
            printf("u: %f, v:%f\n", hash[n].uv[x][0], hash[n].uv[x][1]);
        }*/

        if (FT_Load_Char(face, unicode_char[i], FT_LOAD_RENDER)) 
        {
            fprintf(stderr, "Failed to load Glyph\n");
            return -1;
        }

        FT_Bitmap* bitmap = &face->glyph->bitmap;

        int xOffset = (imageWidth - bitmap->width) / 2;
        int yOffset = (imageHeight - bitmap->rows) / 2;
        
        for (int y = 0; y < bitmap->rows; y++) 
        {
            for (int x = 0; x < bitmap->width; x++) 
            {
                int bufferIndex = ((y + yOffset) * imageWidth + (x + xOffset)) + i * imageWidth * imageHeight;
                unsigned char grayValue = bitmap->buffer[y * bitmap->width + x];
            
                if (grayValue)
                {
                    buffer[bufferIndex] = 255;
                }
                else
                {
                    buffer[bufferIndex] = 0;
                }
            }
        }
    }
    
    unsigned char * buffer2 = (unsigned char*)calloc(imageWidth * imageHeight * line * characterPerLine, sizeof(unsigned char));
    for (int y = 0;y < line;y++)
    {
        for (int i = 0;i < characterPerLine;i++)
        {
            for (int x = 0;x < imageHeight;x++)
            {
                memcpy(buffer2 + (x * imageWidth * characterPerLine + i * pixelsPerLine + y * imageHeight * imageWidth * characterPerLine), buffer + (imageWidth * imageHeight * i + x * pixelsPerLine + y * imageHeight * imageWidth * characterPerLine), sizeof(unsigned char) * pixelsPerLine);
            }
        }
    }
    free(buffer);

    if ((fp = fopen(hashPath, "wb")) == NULL)
    {
        printf("hash map not saved\n");
    }
    else
    {
        fwrite(hash, sizeof(Hash), HASH_SIZE, fp);
        fclose(fp);
    }

    stbi_write_png("character.png", imageWidth * characterPerLine, imageHeight * line, 1, buffer2, imageWidth * characterPerLine);

    free(buffer2);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return 0;
}