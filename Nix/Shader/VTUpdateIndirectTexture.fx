#include "VTCommon.fx"

cbuffer cbPhysPageUpdateIndex : register(b0, space0)
{
    CBufferPhysPageUpdateIndex m_physPageUpdateIndex[UPDATE_INDIRECT_TEXTURE_PER_FRAME];
}

// 使用数组形式，支持动态索引访问不同mip层级
// register(u0)会自动扩展到 u0-u10
RWTexture2D<uint> m_txIndirectTexture[11] : register(u0, space0);

[numthreads(1, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    CBufferPhysPageUpdateIndex pageData = m_physPageUpdateIndex[dtid.x];
    
    if (pageData.index == -1)
        m_txIndirectTexture[pageData.mip][pageData.pageID] = 0xFFFFFFFF;
    else
        m_txIndirectTexture[pageData.mip][pageData.pageID] = pageData.index << 16 | pageData.mip;
}
