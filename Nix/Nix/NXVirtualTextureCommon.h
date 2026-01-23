#pragma once
#include <BaseDefs/NixCore.h>
#include <BaseDefs/Math.h>

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
