#include "text.h"
#include "log.h"

static Hash hash[2][HASH_SIZE];

bool initTextSystem(void)
{
    FILE * fp;
    char fileLocation[100];
    strcpy(fileLocation, __argv[0]);
    strcat(fileLocation, "\\hash-table");
    if ((fp = fopen(fileLocation, "rb")) == NULL)
    {
        logMessage("cannot open char hash file");
        return 0;
    }

    fread(hash, sizeof(Hash), HASH_SIZE * 2, fp);
    fclose(fp);
    // for (int x = 0;x < 2;x++)
    // for (int i = 0;i < 5000;i++)
    // {
    //     logMessage("uv: %f", hash[x][i].uv[0][0]);
    // }

   return true;
}

static uint32_t hash_func1(unistring_uint32_t utf32)
{
    return (utf32 * 2654435761U) % HASH_SIZE;
}

static uint32_t hash_func2(unistring_uint32_t utf32)
{
    return (utf32 * 3344921057U) % HASH_SIZE;
}

typedef struct _Index
{
    int a;
    uint32_t b;
} Index;

static Index find_value(unistring_uint32_t utf32)
{
    uint32_t pos1 = hash_func1(utf32);

    if (hash[0][pos1].utf32 == utf32)
    {
        return (Index){0, pos1};
    }
    
    uint32_t pos2 = hash_func2(utf32);
    if (hash[1][pos2].utf32 == utf32)
    {
        return (Index){1, pos2};
    }
    else
    {
        return (Index){-1, 0};
    }
}

vec2 UVs[100][4];

void getTextUV(char * text, uint32_t * textLen)
{
    size_t u8Len = strlen(text);
    unistring_uint32_t u32Result[u8Len];
    size_t u32Len;
    u8_to_u32((unistring_uint8_t*)text, u8Len, u32Result, &u32Len);
    memset(UVs, 0, sizeof(UVs));
    for (size_t i = 0;i < u32Len;i++)
    {
        Index index = find_value(u32Result[i]);
        logMessage("U+%X, index: %d", u32Result[i], index.b);
        if (index.a != -1)
        {
            for (int x = 0;x < 4;x++)
            {
                UVs[i][x][0] = hash[index.a][index.b].uv[x][0];
                UVs[i][x][1] = hash[index.a][index.b].uv[x][1];
                logMessage("u: %f, v: %f", UVs[i][x][0], UVs[i][x][1]);
            }
        }
    }
    *textLen = u32Len;
}