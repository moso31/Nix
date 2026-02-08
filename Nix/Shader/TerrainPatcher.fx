#include "Common.fx"
#include "TerrainCommon.fx"

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
    
    int patchSize = nodeSize / NXGPUTERRAIN_PATCH_SIZE;
    int2 patchOrigin = nodeOrigin + int2(gtid.x * patchSize, gtid.y * patchSize);
    
    // 这个patcher在node内的相对位置
    float2 patchOriginPixelPos = gtid.xy * VERTEX_GRID_SIZE;
    
    float2 nodeMinMaxZ = m_nodeDescArray[nodeId].minmaxZ;

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
    
    float3 aabbMin = float3(patchOrigin.x, nodeMinMaxZ.x, patchOrigin.y);
    float3 extent = float3(patchSize, (nodeMinMaxZ.y - nodeMinMaxZ.x), patchSize) * 0.5f;
    float3 center = aabbMin + extent;

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
        TerrainPatchData patch = (TerrainPatchData)0;
        patch.atlasIndex = nodeId;
        patch.patchOrigin = patchOrigin;
        patch.patchSize = patchSize;
        patch.nodeOrigin = nodeOrigin;
        patch.patchOriginPixelPos = patchOriginPixelPos;
        
        m_patcherBuffer.Append(patch);
        InterlockedAdd(m_patcherDrawIndexArgs[0].instanceCount, 1);
    }
}
