#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "stream.h"

Level level;

#ifdef __32X__
    extern uint8 gLightmap[256 * 32]; // SDRAM 8k at 0x6000000
#else
    #ifndef MODEHW
        IWRAM_DATA uint8 gLightmap[256 * 32]; // IWRAM 8k
    #endif
#endif

EWRAM_DATA ItemObj items[MAX_ITEMS];

#ifdef ROM_READ
EWRAM_DATA Texture textures[MAX_TEXTURES]; // animated textures require memory swap
EWRAM_DATA Sprite sprites[MAX_SPRITES];
EWRAM_DATA FixedCamera cameras[MAX_CAMERAS];
EWRAM_DATA Box boxes[MAX_BOXES];
#endif

EWRAM_DATA Room rooms[MAX_ROOMS];
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA const Mesh* meshes[MAX_MESHES];
EWRAM_DATA StaticMesh staticMeshes[MAX_STATIC_MESHES];

EWRAM_DATA ItemObj* ItemObj::sFirstActive;
EWRAM_DATA ItemObj* ItemObj::sFirstFree;

EWRAM_DATA int32 gBrightness;

#if (USE_FMT & (LVL_FMT_PHD | LVL_FMT_PSX))
    uint8 gLevelData[2 * 1024 * 1024];
#endif

#if (USE_FMT & LVL_FMT_PKD)
    #include "fmt/pkd.h"
#endif

#if (USE_FMT & LVL_FMT_PHD)
    #include "fmt/phd.h"
#endif

#if (USE_FMT & LVL_FMT_PSX)
    #include "fmt/psx.h"
#endif

bool readLevelStream(Stream& f)
{
#ifdef CPU_BIG_ENDIAN
    f.bigEndian = true;
#endif

#if (USE_FMT & LVL_FMT_PKD)
    if (read_PKD(f))
        return true;
#endif

#if (USE_FMT & LVL_FMT_PHD)
    if (read_PHD(f))
        return true;
#endif

#if (USE_FMT & LVL_FMT_PSX)
    if (read_PSX(f))
        return true;
#endif

    LOG("Unsupported level format\n");
    ASSERT(false);

    return false;
}

void readLevel(const uint8* data)
{
//#ifdef ROM_READ
    dynSectorsCount = 0;
//#endif

    memset(&level, 0, sizeof(level));

    gAnimTexFrame = 0;

    Stream f(data, 0);
    readLevelStream(f);

    // prepare models // TODO prerocess
    memset(models, 0, sizeof(models));
    for (int32 i = 0; i < level.modelsCount; i++)
    {
        const Model* model = level.models + i;
        ASSERT(model->type < MAX_MODELS);
        models[model->type] = *model;
    }
    level.models = models;

    // prepare meshes
    for (int32 i = 0; i < level.meshesCount; i++)
    {
        meshes[i] = (Mesh*)((uint8*)level.meshes + level.meshOffsets[i]);
    }
    level.meshes = meshes;

    //  prepare static meshes // TODO preprocess
    memset(staticMeshes, 0, sizeof(staticMeshes));
    for (int32 i = 0; i < level.staticMeshesCount; i++)
    {
        const StaticMesh* staticMesh = level.staticMeshes + i;

        ASSERT(staticMesh->id < MAX_STATIC_MESHES);
        staticMeshes[staticMesh->id] = *staticMesh;
    }
    level.staticMeshes = staticMeshes;

    // prepare sprites // TODO preprocess
    for (int32 i = 0; i < level.spriteSequencesCount; i++)
    {
        const SpriteSeq* spriteSeq = level.spriteSequences + i;

        if (spriteSeq->type >= TR1_ITEM_MAX) // WTF?
            continue;

        Model* m = models + spriteSeq->type;
        m->count = int8(spriteSeq->count);
        m->start = spriteSeq->start;
    }
}

void animTexturesShift()
{
    const uint16* data = level.animTexData;

    int32 texRangesCount = *data++;

    for (int32 i = 0; i < texRangesCount; i++)
    {
        int32 count = *data++;

        Texture tmp = level.textures[*data];
        while (count > 0)
        {
            level.textures[data[0]] = level.textures[data[1]];
            data++;
            count--;
        }
        level.textures[*data++] = tmp;
    }
}

#define FADING_RATE_SHIFT 4

void updateFading(int32 frames)
{
    if (gBrightness == 0)
        return;

    frames <<= FADING_RATE_SHIFT;

    if (gBrightness < 0)
    {
        gBrightness += frames;
        if (gBrightness > 0) {
            gBrightness = 0;
        }
    }

    if (gBrightness > 0)
    {
        gBrightness -= frames;
        if (gBrightness < 0) {
            gBrightness = 0;
        }
    }

    palSet(level.palette, gSettings.video_gamma << 4, gBrightness);
}

void updateLevel(int32 frames)
{
    updateFading(frames);

    gCausticsFrame += frames;

    gAnimTexFrame += frames;
    while (gAnimTexFrame > 5)
    {
        animTexturesShift();
        gAnimTexFrame -= 5;
    }
}

int32 getAmbientTrack()
{
    return gLevelInfo[gLevelID].track;
}

bool isCutsceneLevel()
{
    return (gLevelID == LVL_TR1_CUT_1) ||
           (gLevelID == LVL_TR1_CUT_2) ||
           (gLevelID == LVL_TR1_CUT_3) ||
           (gLevelID == LVL_TR1_CUT_4);
}

#endif
