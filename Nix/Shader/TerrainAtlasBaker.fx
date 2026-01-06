#include "TerrainCommon.fx"

Texture2D txIn[NodeDescUpdateIndicesNum] : register(t0);
RWTexture2DArray<float> txOutAtlas : register(u0);
RWTexture2DArray<float4> txOutAtlasNormal : register(u1);

cbuffer cbUpdateIndices : register(b0)
{
    CBufferTerrainNodeDescUpdateInfo m_updateIndices[NodeDescUpdateIndicesNum];
}

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelPos = dtid.xy;
    uint inputIndex = dtid.z;
    
    float height = txIn[inputIndex].Load(int3(pixelPos, 0)).r;
    txOutAtlas[uint3(pixelPos, m_updateIndices[inputIndex].newIndex)] = height;
}

[numthreads(8, 8, 1)]
void CS_Float4(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelPos = dtid.xy;
    uint inputIndex = dtid.z;
    
    float4 normal = txIn[inputIndex].Load(int3(pixelPos, 0)).rgba;
    txOutAtlasNormal[uint3(pixelPos, m_updateIndices[inputIndex].newIndex)] = normal;
}