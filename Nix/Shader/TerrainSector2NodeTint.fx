#include "TerrainCommon.fx"

cbuffer cbNodeDescArray : register(b0)
{
    CBufferTerrainNodeDescription m_nodeDescArray[NodeDescArrayNum];
}

cbuffer cbNodeUpdateIndices : register(b1)
{
    CBufferTerrainNodeDescUpdateInfo m_updateIndices[NodeDescUpdateIndicesNum];
}

RWTexture2D<uint> txSector2NodeID_mip[6] : register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{    
    int updateIndex = dtid.x;
    int nodeDescIndex = m_updateIndices[updateIndex].newIndex;

    uint mip = firstbithigh(m_nodeDescArray[nodeDescIndex].size) - 6; // 2048->5, ..., 64->0
    int2 pixelPos = (m_nodeDescArray[nodeDescIndex].positionWS - MinTerrainCoord) >> (6 + mip);

    txSector2NodeID_mip[mip][pixelPos] = nodeDescIndex;

    // 如果nodeDescArray[nodeDescIndex]之前有记录旧值，则需要擦除
    if (m_updateIndices[updateIndex].replaceSize > 0)
    {
        mip = firstbithigh(m_updateIndices[updateIndex].replaceSize) - 6; // 2048->5, ..., 64->0
        pixelPos = (m_updateIndices[updateIndex].replacePosWS - MinTerrainCoord) >> (6 + mip);

        txSector2NodeID_mip[mip][pixelPos] = -1;
    }
}
