#include "Common.fx"
#include "GPUTerrainCommon.fx"

struct NXGPUDrawIndexArgs 
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation; // 0
    int  baseVertexLocation; // 0
    uint startInstanceLocation; // 0
};

Texture2D m_minmaxZMap : register(t0);
SamplerState ssLinearClamp : register(s0);

RWStructuredBuffer<uint3> m_terrainBuffer : register(u0);
AppendStructuredBuffer<NXGPUTerrainPatch> m_patchBuffer : register(u1);
RWStructuredBuffer<NXGPUDrawIndexArgs> m_drawIndexArgs : register(u2);
RWByteAddressBuffer m_patchBufferUAVCounter : register(u3); // uav counter of m_patchBuffer!
   
#define NXGPUTERRAIN_PATCH_SIZE 8

[numthreads(1, 1, 1)]
void CS_Clear(uint3 dtid : SV_DispatchThreadID)
{
    m_drawIndexArgs[0] = (NXGPUDrawIndexArgs)0;
    m_drawIndexArgs[0].indexCountPerInstance = 24576;
    //m_drawIndexArgs[0].instanceCount = 0;
    m_patchBufferUAVCounter.Store(0, 0);
}

[numthreads(NXGPUTERRAIN_PATCH_SIZE, NXGPUTERRAIN_PATCH_SIZE, 1)]
void CS_Patch(
    uint3 dtid : SV_DispatchThreadID, 
    uint3 gtid : SV_GroupThreadID,
    uint groupIndex : SV_GroupID
)
{
    // z : lod等级；xy : 此lod等级下 xy偏移量
    uint3 param = m_terrainBuffer[groupIndex];
    float scale = (float)(1u << (5 - param.z)) * 1.0 / (float)(NXGPUTERRAIN_PATCH_SIZE);

    // 计算实际世界坐标 for block and patch
    float blockSize = (float)(2048u >> param.z);
    float patchSize = blockSize / NXGPUTERRAIN_PATCH_SIZE;

    float3 blockOrigin = float3(param.x, 0.0f, param.y) * blockSize;
    float3 patchOrigin = blockOrigin + float3(gtid.x, 0, gtid.y) * patchSize;

    matrix mxScale = matrix(
		scale, 0.0f, 0.0f, 0.0f,
		0.0f, scale, 0.0f, 0.0f,
		0.0f, 0.0f, scale, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

    NXGPUTerrainPatch patch = (NXGPUTerrainPatch)0;
    patch.pos = patchOrigin;
    patch.mxWorld = matrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		patchOrigin.x, patchOrigin.y, patchOrigin.z, 1.0f
	);

    patch.mxWorld = mul(mxScale, patch.mxWorld);
    patch.uv = (float)gtid.xy;
    patch.pad = (float) (5 - param.z);

    // visibility test
    // Frustum Culling
    float4 plane[6];
    plane[0] = NormalizePlane(m_viewProjection[0] - m_viewProjection[3]);
    plane[1] = NormalizePlane(m_viewProjection[0] + m_viewProjection[3]);
    plane[2] = NormalizePlane(m_viewProjection[1] - m_viewProjection[3]);
    plane[3] = NormalizePlane(m_viewProjection[1] + m_viewProjection[3]);
    plane[4] = NormalizePlane(m_viewProjection[2]);
    plane[5] = NormalizePlane(m_viewProjection[2] - m_viewProjection[3]);

    bool isoutside = false;
    for (int i = 0; i < 6; i++)
    {
        float3 n = plane[i].xyz;
        float d = plane[i].w;

        float3 extent = float3(patchSize * 0.5f, 0.0f, patchSize * 0.5f);
        float3 center = patch.pos + extent;

        float s = dot(n, center) + d;
        float r = dot(abs(n), extent);

        if (s - r > 0)
        {
            isoutside = true;
            break;
        }
    }

    if (!isoutside)
    {
        InterlockedAdd(m_drawIndexArgs[0].instanceCount, 1);
        m_patchBuffer.Append(patch);
    }
}
