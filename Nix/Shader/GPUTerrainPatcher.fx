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

Texture2DArray m_minmaxZMap : register(t0);
SamplerState ssPointClamp : register(s0);

StructuredBuffer<int3> m_terrainBuffer : register(t1);
AppendStructuredBuffer<NXGPUTerrainPatch> m_patchBuffer : register(u0);
RWStructuredBuffer<NXGPUDrawIndexArgs> m_drawIndexArgs : register(u1);
RWByteAddressBuffer m_patchBufferUAVCounter : register(u2); // uav counter of m_patchBuffer!

cbuffer cbTerrainSupport : register(b2)
{
    int m_blockMinIdX;
    int m_blockMinIdY;
    int m_blockCountX;
}
   
#define MINMAXZ_TEXSIZE 256u // TODO：临时的，如果多地形不见得能这么干……

#define TERRAIN_SIZE 2048u
#define NXGPUTERRAIN_PATCH_SIZE 8u

[numthreads(1, 1, 1)]
void CS_Clear(uint3 dtid : SV_DispatchThreadID)
{
    m_drawIndexArgs[0] = (NXGPUDrawIndexArgs)0;
    m_drawIndexArgs[0].indexCountPerInstance = 384;
    //m_drawIndexArgs[0].instanceCount = 0;
    m_patchBufferUAVCounter.Store(0, 0);
}

[numthreads(NXGPUTERRAIN_PATCH_SIZE, NXGPUTERRAIN_PATCH_SIZE, 1)]
void CS_Patch(
    uint3 dtid : SV_DispatchThreadID, 
    uint3 gtid : SV_GroupThreadID,
    uint3 groupIndex : SV_GroupID
)
{
    // z : lod等级；xy : 此lod等级下 xy偏移量
    int3 param = m_terrainBuffer[groupIndex.x];
    uint mip = 5u - param.z;
    float scale = (float)(1u << mip) * 1.0f / (float)(NXGPUTERRAIN_PATCH_SIZE);

    // 计算实际世界坐标 for block and patch
    float blockSize = (float)(TERRAIN_SIZE >> param.z);
    float patchSize = blockSize / (float)NXGPUTERRAIN_PATCH_SIZE;

    float3 blockOrigin = float3(param.x, 0.0f, param.y) * blockSize;
    float3 patchOrigin = blockOrigin + float3(gtid.x, 0, gtid.y) * patchSize;

    matrix mxScale = matrix(
		scale, 0.0f, 0.0f, 0.0f,
		0.0f, scale, 0.0f, 0.0f,
		0.0f, 0.0f, scale, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
    
    int2 coord = (param.xy >> param.z) - int2(m_blockMinIdX, m_blockMinIdY);
    int sliceIndex = coord.y * m_blockCountX + coord.x;

    float2 patchUV = frac((patchOrigin.xz + patchSize * 0.5) / (float)TERRAIN_SIZE);
    float2 minMaxZ = m_minmaxZMap.SampleLevel(ssPointClamp, float3(patchUV.x, 1.0 - patchUV.y, (float)sliceIndex), 0);
    float yExtent = (minMaxZ.y - minMaxZ.x);
    float yCenter = (minMaxZ.y + minMaxZ.x) * 0.5f;

    NXGPUTerrainPatch patch = (NXGPUTerrainPatch)0;
    patch.pos = patchOrigin;
    patch.mxWorld = matrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		patchOrigin.x, patchOrigin.y, patchOrigin.z, 1.0f
	);

    patch.mxWorld = mul(mxScale, patch.mxWorld);
    patch.uv = minMaxZ;
    
    patch.sliceIndex = sliceIndex;
    patch.terrainOrigin = floor(blockOrigin.xz / (float)TERRAIN_SIZE) * (float)TERRAIN_SIZE;

    // visibility test: Frustum Culling
    float4 plane[6];
    matrix vp = transpose(m_viewProjection);
    plane[0] = NormalizePlane(vp[3] - vp[0]);
    plane[1] = NormalizePlane(vp[3] + vp[0]);
    plane[2] = NormalizePlane(vp[3] - vp[1]);
    plane[3] = NormalizePlane(vp[3] + vp[1]);
    plane[4] = NormalizePlane(vp[2]);
    plane[5] = NormalizePlane(vp[3] - vp[2]);

    //for (int i = 0; i < 6; ++i) plane[i].w -= 100.0f; // debug

    float3 extent = float3(patchSize * 0.5f, yExtent * 0.5f, patchSize * 0.5f);
    float3 center = patch.pos + float3(0.0f, minMaxZ.x, 0.0f) + extent;

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
        m_patchBuffer.Append(patch);
        InterlockedAdd(m_drawIndexArgs[0].instanceCount, 1);
    }
}
