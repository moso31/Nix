#include "GPUTerrainCommon.fx"

struct NXGPUDrawIndexArgs 
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation; // 0
    int  baseVertexLocation; // 0
    uint startInstanceLocation; // 0
};

RWStructuredBuffer<uint3> m_terrainBuffer : register(u0);
AppendStructuredBuffer<NXGPUTerrainPatch> m_patchBuffer : register(u1);
RWStructuredBuffer<NXGPUDrawIndexArgs> m_drawIndexArgs : register(u2);
RWByteAddressBuffer m_patchBufferUAVCounter : register(u3); // uav counter of m_patchBuffer!
   
#define NXGPUTERRAIN_PATCH_SIZE 1

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
    float scale = (float)(1u << (5 - param.z));

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
    patch.pad = (float)param.z;

    // todo: if patch visible...
    InterlockedAdd(m_drawIndexArgs[0].instanceCount, 1);

    m_patchBuffer.Append(patch);
}
