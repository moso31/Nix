#pragma once
#include <BaseDefs/NixCore.h>
#include <BaseDefs/Math.h>
#include "NXTerrainCommon.h"

struct NXVTLRUKey
{
    uint64_t GetKey()
    {
        Int2 sectorOffset = sector - g_terrainConfig.MinSectorID;
        return static_cast<size_t>(sectorOffset.x) << 40 | static_cast<size_t>(sectorOffset.y) << 32 | pageID.x << 20 | pageID.y << 8 | gpuMip << 4 | indiTexLog2Size;
    }

    Int2 sector;
    Int2 pageID;
    int gpuMip;
    int indiTexLog2Size;
    Int2 _0;
};

static inline int VTImageIndexDecode(int x)
{
    x &= 0x55555555u;
    x = (x | (x >> 1)) & 0x33333333u;
    x = (x | (x >> 2)) & 0x0F0F0F0Fu;
    x = (x | (x >> 4)) & 0x00FF00FFu;
    x = (x | (x >> 8)) & 0x0000FFFFu;
    return x;
}

static inline Int2 VTImageIndexToPos(int idx)
{
    return Int2(VTImageIndexDecode(idx >> 1), VTImageIndexDecode(idx));
}

static inline int VTImageIndexEncode(int x)
{
    x &= 0x0000FFFFu;
    x = (x | (x << 8)) & 0x00FF00FFu;
    x = (x | (x << 4)) & 0x0F0F0F0Fu;
    x = (x | (x << 2)) & 0x33333333u;
    x = (x | (x << 1)) & 0x55555555u;
    return x;
}

static inline int VTImagePosToIndex(const Int2& p)
{
    return (VTImageIndexEncode(p.x) << 1) | VTImageIndexEncode(p.y);
}

// HLSL cbuffer 数组元素需要 16 字节对齐
struct CBufferPhysPageUpdateIndex
{
    CBufferPhysPageUpdateIndex(int index, Int2 pageID, int mip) : index(index), pageID(pageID), mip(mip) {}
    int index;
    Int2 pageID;
    int mip;
};

struct NXVirtualTextureConfig
{
    uint32_t PhysicalPageTilePadding = 4;
    uint32_t PhysicalPageTileSize = 256 + PhysicalPageTilePadding * 2;
    uint32_t PhysicalPageTileNum = 1024;

    uint32_t IndirectTextureSize = 2048;
};

inline NXVirtualTextureConfig g_virtualTextureConfig;
