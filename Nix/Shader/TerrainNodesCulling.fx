#define NodeDescArrayNum 1024

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

ConsumeStructuredBuffer<uint> m_inBuffer : register(u0);
AppendStructuredBuffer<uint> m_outBuffer : register(u1);
AppendStructuredBuffer<uint> m_final : register(u2);

Texture2D<uint> m_txSector2NodeID : register(t0);

cbuffer cbNodeDescArray : register(b0)
{
    CBufferTerrainNodeDescription m_nodeDescArray[NodeDescArrayNum];
}

cbuffer cbLodDist : register(b1)
{
    float3 m_cameraPos;
    float m_currentLodDist;
    int m_currentMip;
    
    float3 pad;
}

[numthreads(8, 8, 1)]
void CS_First(uint3 dtid : SV_DispatchThreadID)
{    
    // 第一个pass获取所有最低精度mip的子节点id
    uint2 pixelPos = dtid.xy;
    uint nodeId = m_txSector2NodeID.Load(int3(pixelPos, m_currentMip));
    m_outBuffer.Append(nodeId);
}

[numthreads(1, 1, 1)]
void CS_Process()
{
    // 从父节点取一个node
    uint nodeID = m_inBuffer.Consume();
    CBufferTerrainNodeDescription nodeDesc = m_nodeDescArray[nodeID];
    
    // 获取父节点node对应的像素坐标
    int2 pixelPos = (nodeDesc.positionWS - MinTerrainCoord) >> (6 + m_currentMip);
    
    // 子节点的坐标
    int2 childPixelPos0 = pixelPos * 2 + int2(0, 0);
    int2 childPixelPos1 = pixelPos * 2 + int2(0, 1);
    int2 childPixelPos2 = pixelPos * 2 + int2(1, 0);
    int2 childPixelPos3 = pixelPos * 2 + int2(1, 1);
    
    uint childNodeID0 = m_txSector2NodeID.Load(int3(childPixelPos0, m_currentMip - 1));
    uint childNodeID1 = m_txSector2NodeID.Load(int3(childPixelPos1, m_currentMip - 1));
    uint childNodeID2 = m_txSector2NodeID.Load(int3(childPixelPos2, m_currentMip - 1));
    uint childNodeID3 = m_txSector2NodeID.Load(int3(childPixelPos3, m_currentMip - 1));
    
    // 如果是最后一次迭代 或者是无效节点0xffff
    if (m_currentMip == 0 || childNodeID0 == 0xffff || childNodeID1 == 0xffff || childNodeID2 == 0xffff || childNodeID3 == 0xffff) 
    {
        // 当前节点计入final
        m_final.Append(nodeID);
        return;
    }
    
    // 若超过相机规划的mip距离，是否跳过迭代
    {
        CBufferTerrainNodeDescription childNode0 = m_nodeDescArray[childNodeID0];
        CBufferTerrainNodeDescription childNode1 = m_nodeDescArray[childNodeID1];
        CBufferTerrainNodeDescription childNode2 = m_nodeDescArray[childNodeID2];
        CBufferTerrainNodeDescription childNode3 = m_nodeDescArray[childNodeID3];
    
        float2 childCenter0 = childNode0.positionWS + childNode0.size / 2;
        float2 childCenter1 = childNode1.positionWS + childNode1.size / 2;
        float2 childCenter2 = childNode2.positionWS + childNode2.size / 2;
        float2 childCenter3 = childNode3.positionWS + childNode3.size / 2;
    
        float childDistance0 = distance(m_cameraPos.xz, childCenter0);
        float childDistance1 = distance(m_cameraPos.xz, childCenter1);
        float childDistance2 = distance(m_cameraPos.xz, childCenter2);
        float childDistance3 = distance(m_cameraPos.xz, childCenter3);
    
        if (childDistance0 > m_currentLodDist || childDistance1 > m_currentLodDist || childDistance2 > m_currentLodDist || childDistance3 > m_currentLodDist)
        {
            // 当前节点计入final
            m_final.Append(nodeID);
            return;
        }
    }
    
    m_outBuffer.Append(childNodeID0);
    m_outBuffer.Append(childNodeID1);
    m_outBuffer.Append(childNodeID2);
    m_outBuffer.Append(childNodeID3);
}