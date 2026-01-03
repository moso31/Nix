#include "Common.fx"

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

struct TerrainPatchData
{
    matrix mxWorld;
    int2 patchOrigin;
    int2 patchSize;
    int sliceIndex;
    
    float3 _pad0;
};

struct TerrainPatchDrawIndexArgs 
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation; // 0
    int  baseVertexLocation; // 0
    uint startInstanceLocation; // 0
};

StructuredBuffer<uint> m_finalNodeIdArray : register(t0);
AppendStructuredBuffer<TerrainPatchData> m_patcherBuffer : register(u0);
RWStructuredBuffer<TerrainPatchDrawIndexArgs> m_patcherDrawIndexArgs : register(u1);

#define NodeDescArrayNum 1024
cbuffer cbNodeDescArray : register(b0)
{
    CBufferTerrainNodeDescription m_nodeDescArray[NodeDescArrayNum];
}

#define VERTEX_GRID_SIZE 8 // 根据NXSubMeshTerrain的顶点调整
#define NXGPUTERRAIN_PATCH_SIZE 8
[numthreads(NXGPUTERRAIN_PATCH_SIZE, NXGPUTERRAIN_PATCH_SIZE, 1)]
void CS(
    uint3 gtid : SV_GroupThreadID,
    uint3 groupIndex : SV_GroupID)
{
    uint nodeId = m_finalNodeIdArray[groupIndex.x];
    int2 nodeOrigin = m_nodeDescArray[nodeId].positionWS;
    int nodeSize = m_nodeDescArray[nodeId].size;
    
    int2 patchOrigin = nodeOrigin / NXGPUTERRAIN_PATCH_SIZE;
    int patchSize = nodeSize / NXGPUTERRAIN_PATCH_SIZE;
    
    int2 nodeMinMaxZ = m_nodeDescArray[nodeId].minmaxZ;

    // visibility test: Frustum Culling
    float4 plane[6];
    matrix vp = transpose(m_viewProjection);
    plane[0] = NormalizePlane(vp[3] - vp[0]);
    plane[1] = NormalizePlane(vp[3] + vp[0]);
    plane[2] = NormalizePlane(vp[3] - vp[1]);
    plane[3] = NormalizePlane(vp[3] + vp[1]);
    plane[4] = NormalizePlane(vp[2]);
    plane[5] = NormalizePlane(vp[3] - vp[2]);

    //for (int i = 0; i < 6; ++i) plane[i].w -= m_debugParam; // debug

    float3 extent = float3(patchSize, nodeMinMaxZ.y - nodeMinMaxZ.x, patchSize);
    float3 center = float3(patchOrigin.x, 0.0f, patchOrigin.y) + extent * 0.5f;

    bool isoutside = false;
    for (int i = 0; i < 6; i++)
    {
        float3 n = plane[i].xyz;
        float d = plane[i].w;

        float s = dot(n, center) + d;
        float r = dot(abs(n), extent);

        if (s + r < 0)
        {
            isoutside = true;
            break;
        }
    }

    if (!isoutside)
    {
        float scale = (float)(patchSize / VERTEX_GRID_SIZE);
        TerrainPatchData patch = (TerrainPatchData)0;
        patch.sliceIndex = nodeId;
        patch.patchOrigin = patchOrigin;
        patch.patchSize = patchSize;
        patch.mxWorld = matrix(
		    scale, 0.0f, 0.0f, 0.0f,
		    0.0f, scale, 0.0f, 0.0f,
		    0.0f, 0.0f, scale, 0.0f,
		    (float)patchOrigin.x, 0.0f, (float)patchOrigin.y, 1.0f
	    );
        
        m_patcherBuffer.Append(patch);
        InterlockedAdd(m_patcherDrawIndexArgs[0].instanceCount, 1);
    }
}
