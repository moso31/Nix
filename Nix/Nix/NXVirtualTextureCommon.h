#pragma once
#include <BaseDefs/NixCore.h>
#include <BaseDefs/Math.h>
#include "NXTerrainCommon.h"

struct NXVTLRUKey
{
    NXVTLRUKey() = default;

    NXVTLRUKey(uint64_t hash)
    {
        sector = Int2(
            int((hash >> 40) & 0xFF),
            int((hash >> 32) & 0xFF)
        ) + g_terrainConfig.MinSectorID;

        pageID = Int2(
            int((hash >> 20) & 0xFFF),
            int((hash >> 8) & 0xFFF)
        );

        gpuMip = int((hash >> 4) & 0xF);
        indiTexLog2Size = int((hash >> 0) & 0xF);
        bakeIndirectTextureIndex = -1;
    }

    uint64_t GetKey()
    {
        Int2 sectorOffset = sector - g_terrainConfig.MinSectorID;

        return ((uint64_t(sectorOffset.x) & 0xFFull) << 40) |
            ((uint64_t(sectorOffset.y) & 0xFFull) << 32) |
            ((uint64_t(pageID.x) & 0xFFFull) << 20) |
            ((uint64_t(pageID.y) & 0xFFFull) << 8) |
            ((uint64_t(gpuMip) & 0xFull) << 4) |
            ((uint64_t(indiTexLog2Size) & 0xFull));
    }

    Int2 sector;
    Int2 pageID;
    int gpuMip;
    int indiTexLog2Size;
    int bakeIndirectTextureIndex;
    int _0;
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
