static const uint g_Bayer8x8[64] =
{
    0, 48, 12, 60, 3, 51, 15, 63,
    32, 16, 44, 28, 35, 19, 47, 31,
     8, 56, 4, 52, 11, 59, 7, 55,
    40, 24, 36, 20, 43, 27, 39, 23,
     2, 50, 14, 62, 1, 49, 13, 61,
    34, 18, 46, 30, 33, 17, 45, 29,
    10, 58, 6, 54, 9, 57, 5, 53,
    42, 26, 38, 22, 41, 25, 37, 21
};

struct CBufferPhysPageUpdateIndex
{
    int index;
    int2 pageID;
    int mip;
};

struct CBufferRemoveSector
{
    int2 imagePos;
    int imageSize;
    int maxRemoveMip; // 值=N，表示只移除前N个mip（0~N-1）
};

struct CBufferMigrateSector
{
    int2 fromImagePos;
    int2 toImagePos;
    int fromImageSize;
    int toImageSize;
    int mipDelta; // 迁移前后两个sector的mip等级差
    int _0;
};

#define UPDATE_INDIRECT_TEXTURE_PER_FRAME 1024
#define BAKE_PHYSICAL_PAGE_PER_FRAME 32
#define BAKE_PHYSICAL_PAGE_SIZE 256
#define BAKE_PHYSICAL_PAGE_BORDER 4
#define SECTOR_SIZE 64
#define SECTOR_MIN int2(-128, -128)
#define NodeDescArrayNum 1024

float MipLevelPoint(float2 uv, float size)
{
    float2 dX = ddx(uv * size);
    float2 dY = ddy(uv * size);
    float delta = max(dot(dX, dX), dot(dY, dY));
    return ceil(0.5f * log2(delta));
}

float MipLevelLinear(float2 uv, float size)
{
    float2 dX = ddx(uv * size);
    float2 dY = ddy(uv * size);
    float delta = max(dot(dX, dX), dot(dY, dY));
    return 0.5f * log2(delta);
}

// From https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm
float MipLevelAnisotropy(float2 uv, float size)
{
    float2 dX = ddx(uv * size);
    float2 dY = ddy(uv * size);
    float squaredLengthX = dot(dX, dX);
    float squaredLengthY = dot(dY, dY);
    float determinant = abs(dX.x * dY.y - dX.y * dY.x);
    bool isMajorX = squaredLengthX > squaredLengthY;
    float squaredLengthMajor = isMajorX ? squaredLengthX : squaredLengthY;
    float lengthMajor = sqrt(squaredLengthMajor);
    float normMajor = 1.f / lengthMajor;

    float2 anisoLineDirection;
    anisoLineDirection.x = (isMajorX ? dX.x : dY.x) * normMajor;
    anisoLineDirection.y = (isMajorX ? dX.y : dY.y) * normMajor;

    float ratioOfAnisotropy = squaredLengthMajor / determinant;

    // clamp ratio and compute LOD
    float lengthMinor;
    const float maxAniso = 8;
    if (ratioOfAnisotropy > maxAniso) // maxAniso comes from a Sampler state.
    {
        // ratio is clamped - LOD is based on ratio (preserves area)
        ratioOfAnisotropy = maxAniso;
        lengthMinor = lengthMajor / ratioOfAnisotropy;
    }
    else
    {
        // ratio not clamped - LOD is based on area
        lengthMinor = determinant / lengthMajor;
    }

    // clamp to top LOD
    if (lengthMinor < 1.0)
    {
        ratioOfAnisotropy = max(1.0, ratioOfAnisotropy * lengthMinor);

        // lengthMinor = 1.0 // This line is no longer recommended for future hardware
        //
        // The commented out line above was part of the D3D10 spec until 8/17/2009,
        // when it was finally noticed that it was undesirable.
        //
        // Consider the case when the LOD is negative (lengthMinor less than 1),
        // but a positive LOD bias will be applied later on due to
        // sampler / instruction settings.
        //
        // With the clamp of lengthMinor above, the log2() below would make a
        // negative LOD become 0, after which any LOD biasing would apply later.
        // That means with biasing, LOD values less than the bias amount are
        // unavailable.  This would look blurrier than isotropic filtering,
        // which is obviously incorrect.  The output of this routine must allow
        // negative LOD values, so that LOD bias (if used) can still result in
        // hitting the most detailed mip levels.
        //
        // Because this issue was only noticed years after the D3D10 spec was originally
        // authored, many implementations will include a clamp such as commented out
        // above.  WHQL must therefore allow implementations that support either
        // behavior - clamping or not.  It is recommended that future hardware
        // does not do the clamp to 1.0 (thus allowing negative LOD).
        // The same applies for D3D11 hardware as well, since even the D3D11 specs
        // had already been locked down for a long time before this issue was uncovered.
    }

    float LOD = log2(lengthMinor);
    return LOD;
}

uint3 DecodeSector2VirtImgData(int v)
{
    int x = (v >> 20) & 0xFFF; // 12 bit
    int y = (v >> 8) & 0xFFF; // 12 bit
    int log2Size = (v >> 0) & 0x0FF; // 8 bit
    return uint3(x, y, log2Size);
}

uint EncodeIndirectTextureData(uint2 indiTexPos, uint gpuMip, uint indiTexLog2Size)
{
    return ((indiTexPos.x & 0xFFF) << 20) | ((indiTexPos.y & 0xFFF) << 8) | ((gpuMip & 0xF) << 4) | indiTexLog2Size; 
}