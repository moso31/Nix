struct NXGPUTerrainPatch
{
    matrix mxWorld;
    float3 pos;
    float pad;
    float2 uv;
};

RWStructuredBuffer<uint3> m_terrainBuffer : register(u0);
AppendStructuredBuffer<NXGPUTerrainPatch> m_patchBuffer : register(u1);
   
#define NXGPUTERRAIN_PATCH_SIZE 8

[numthreads(NXGPUTERRAIN_PATCH_SIZE, NXGPUTERRAIN_PATCH_SIZE, 1)]
void CS_Patch(
    uint3 dispatchThreadID : SV_DispatchThreadID, 
    uint3 groupThreadID : SV_GroupThreadID,
    uint groupIndex : SV_GroupID
)
{
    // z : lod等级；xy : 此lod等级下 xy偏移量
    uint3 param = m_terrainBuffer[groupIndex];

    // 计算实际世界坐标 for block and patch
    float blockSize = (float)(2048u >> param.z);
    float patchSize = blockSize / NXGPUTERRAIN_PATCH_SIZE;

    float3 blockOrigin = float3(param.x, 0.0f, param.y) * blockSize;
    float3 patchOrigin = blockOrigin + float3(groupThreadID.x, 0, groupThreadID.y) * patchSize;

    NXGPUTerrainPatch patch = (NXGPUTerrainPatch)0;
    patch.pos = patchOrigin;
    patch.mxWorld = matrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		patchOrigin.x, patchOrigin.y, patchOrigin.z, 1.0f
	);

    m_patchBuffer.Append(patch);
}
