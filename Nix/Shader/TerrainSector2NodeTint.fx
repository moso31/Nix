#define NodeDescArrayNum 1024
#define NodeDescUpdateIndicesNum 4

// 最小地形坐标，按说应该传个CB进来，先不搞了，图省事
#define MinTerrainCoord -8192

struct CBufferTerrainNodeDescription
{
	// 地形左下角XZ节点坐标（左手坐标系）
    int2 positionWS;

	// 节点的minmaxZ数据
    float2 minmaxZ;

	// 节点大小，一定是2的整数幂
    uint size;

    uint3 padding; // 16 byte align
};

struct CBufferTerrainNodeDescUpdateInfo
{
	// 要替换的索引
    int newIndex;

	// 被替换的旧信息和大小
	// 注意如果是不需要replace，则size = 0;
    int2 replacePosWS;
    int replaceSize;
};

cbuffer cbNodeDescArray : register(b0)
{
    CBufferTerrainNodeDescription m_nodeDescArray[NodeDescArrayNum];
}

cbuffer cbNodeUpdateIndices : register(b1)
{
    CBufferTerrainNodeDescUpdateInfo m_updateIndices[NodeDescUpdateIndicesNum];
}

RWTexture2D<uint> txSector2NodeID : register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{    
    int updateIndex = dtid.x;
    int nodeDescIndex = m_updateIndices[updateIndex].newIndex;

    uint mip = firstbithigh(m_nodeDescArray[nodeDescIndex].size) - 6; // 2048->5, ..., 64->0
    int2 pixelPos = (m_nodeDescArray[nodeDescIndex].positionWS - MinTerrainCoord) >> (6 + mip);

    txSector2NodeID[uint3(pixelPos, mip)] = nodeDescIndex;

    // 如果nodeDescArray[nodeDescIndex]之前有记录旧值，则需要擦除
    if (m_updateIndices[updateIndex].replaceSize > 0)
    {
        mip = firstbithigh(m_updateIndices[updateIndex].replaceSize) - 6; // 2048->5, ..., 64->0
        pixelPos = (m_updateIndices[updateIndex].replacePosWS - MinTerrainCoord) >> (6 + mip);

        txSector2NodeID[uint3(pixelPos, mip)] = -1;
    }
}
