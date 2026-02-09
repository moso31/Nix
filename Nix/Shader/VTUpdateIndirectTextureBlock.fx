#include "VTCommon.fx"

cbuffer cbRemoveSector : register(b0, space0)
{
    CBufferRemoveSector m_removeData;
}

cbuffer cbMigrateSector : register(b0, space0)
{
    CBufferMigrateSector m_migrateData;
}

// 使用数组形式，支持动态索引访问不同mip层级
// register(u0)会自动扩展到 u0-u10
RWTexture2D<uint> m_txIndirectTexture[11] : register(u0, space0);

[numthreads(8, 8, 1)]
void CS_Remove(uint3 dtid : SV_DispatchThreadID)
{
    int2 coord = (int2)dtid.xy;
    
    int mip = 0;
    int2 base = m_removeData.imagePos * m_removeData.imageSize;
    
    [unroll(11)]
    for (int size = m_removeData.imageSize; size > 0; size >>= 1)
    {
        if (mip >= m_removeData.maxRemoveMip)
            break;
        
        if (all(coord < size))
        {
            int2 removePixel = (base >> mip) + coord;
            m_txIndirectTexture[mip][removePixel] = 0xFFFF;
        }
        mip++;
    }
}

[numthreads(8, 8, 1)]
void CS_Migrate(uint3 dtid : SV_DispatchThreadID)
{
    int2 coord = (int2)dtid.xy;
    int2 fromBase = m_migrateData.fromImagePos * m_migrateData.fromImageSize;
    int2 toBase = m_migrateData.toImagePos * m_migrateData.toImageSize;
    
    // upscale
    if (m_migrateData.fromImageSize < m_migrateData.toImageSize)
    {
        int fromMip = 0;
        int toMip = m_migrateData.mipDelta;
        for (int size = m_migrateData.fromImageSize; size > 0; size >>= 1)
        {
            if (all(coord < size))
            {
                int2 fromPixel = (fromBase >> fromMip) + coord;
                int2 toPixel = (toBase >> toMip) + coord;
                m_txIndirectTexture[toMip][toPixel] = m_txIndirectTexture[fromMip][fromPixel];
                m_txIndirectTexture[fromMip][fromPixel] = 0xFFFF;
            }
            fromMip++;
            toMip++;
        }
    }
    else // downscale
    {
        int fromMip = m_migrateData.mipDelta;
        int toMip = 0;
        for (int size = m_migrateData.toImageSize; size > 0; size >>= 1)
        {
            if (all(coord < size))
            {
                int2 fromPixel = (fromBase >> fromMip) + coord;
                int2 toPixel = (toBase >> toMip) + coord;
                m_txIndirectTexture[toMip][toPixel] = m_txIndirectTexture[fromMip][fromPixel];
                m_txIndirectTexture[fromMip][fromPixel] = 0xFFFF;
            }
            fromMip++;
            toMip++;
        }
    }
}
