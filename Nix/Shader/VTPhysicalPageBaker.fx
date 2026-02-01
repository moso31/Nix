#include "VTCommon.fx"

float4 RectGrid(float2 uv, out float2 vertexPosX, out float2 vertexPosY, out float2 vertexPosZ, out float2 vertexPosW, float scale)
{
    float t = scale;
    const float2 base = floor(uv / t) * t;
    vertexPosX = base;
    vertexPosY = base + float2(0, t);
    vertexPosZ = base + float2(t, 0);
    vertexPosW = base + float2(t, t);

    float2 offset = uv - base;
    float2 oneMinusOffset = t - offset;

    float4 result = float4(oneMinusOffset.x * oneMinusOffset.y, oneMinusOffset.x * offset.y, offset.x * oneMinusOffset.y, offset.x * offset.y);
    return result / t / t;
} 

// NXVTLRUKey 对应的结构
struct VTLRUKey
{
	int2 sector;
	int2 pageID;
	int gpuMip;
	int indiTexLog2Size;
	int2 _padding;
};

// CBufferTerrainNodeDescription 对应的结构
struct TerrainNodeDescription
{
	int2 positionWS;
	float2 minmaxZ;
	uint size;
	uint3 padding;
};

cbuffer cbPhysPageBakeData : register(b0, space0)
{
    VTLRUKey m_physPageBakeData[BAKE_PHYSICAL_PAGE_PER_FRAME];
}

cbuffer cbNodeDescArray : register(b1, space0)
{
	TerrainNodeDescription m_nodeDescArray[NodeDescArrayNum];
}

cbuffer cbPhysPageUpdateIndex : register(b2, space0)
{
    CBufferPhysPageUpdateIndex m_physPageUpdateIndex[BAKE_PHYSICAL_PAGE_PER_FRAME];
}

Texture2D<uint> m_sector2NodeIDTex : register(t0, space0);
Texture2DArray<float4> m_terrainSplatMap : register(t1, space0);
Texture2DArray<float4> m_txTerrainBaseColor : register(t2, space0);
Texture2DArray<float4> m_txTerrainNormalMap : register(t3, space0);

RWTexture2DArray<float4> m_txPhysicalPageAlbedo : register(u0, space0);
RWTexture2DArray<float4> m_txPhysicalPageNormal : register(u1, space0);

SamplerState ssPointWrap : register(s0);
SamplerState ssLinearWrap : register(s1);

uint GetBestSector2NodeId(int2 sector)
{
    int2 sectorPixel = sector - SECTOR_MIN; // 采样时偏移至正坐标
    
    int mip = 0;
    while (mip < 6)  // mip0-5, 共6级
    {
        uint nodeID = m_sector2NodeIDTex.Load(int3(sectorPixel, mip));
        if (nodeID != 0xffff)
            return nodeID;
        sectorPixel >>= 1;
        mip++;
    }
    return -1; // 未找到有效节点
}

// dtid: xy = 当前tile内的像素位置; z = 需要烘焙的pageID索引
[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID) 
{
    VTLRUKey key = m_physPageBakeData[dtid.z];
    CBufferPhysPageUpdateIndex updateIdx = m_physPageUpdateIndex[dtid.z];
    int physPageIdx = updateIdx.index;
    // updateIdx.pageID 和 updateIdx.mip 可在需要时使用
    uint nodeID = GetBestSector2NodeId(key.sector);
    
    TerrainNodeDescription nodeDesc = m_nodeDescArray[nodeID];
    float2 sectorPosWS = key.sector * SECTOR_SIZE; 
    float nodeSize = nodeDesc.size - 1.0f; // splat纹理大小是POT+1 需要减回来
    
    int shift = key.indiTexLog2Size - key.gpuMip;
    int2 sectorPageID = key.pageID >> shift << shift;
    
    float2 pageUV = (float2)((int2)(dtid.xy) - BAKE_PHYSICAL_PAGE_BORDER) / BAKE_PHYSICAL_PAGE_SIZE; 
    float2 pageOffset = (float2)key.pageID + pageUV - (float2)sectorPageID;
    float2 mipImageSize = (float2)((1u << key.indiTexLog2Size) >> key.gpuMip);
    float2 imageUV = pageOffset / mipImageSize;
    float2 imageOffset = imageUV * SECTOR_SIZE;
    float2 pixelPosWS = sectorPosWS + imageOffset;
    
    float2 posUV = pixelPosWS - nodeDesc.positionWS;
    float2 posX, posY, posZ, posW;
    float4 weight = RectGrid(posUV, posX, posY, posZ, posW, nodeSize / 64);
    
    float sliceIdx = (float)nodeID;
    float4 splatId;
    splatId.x = m_terrainSplatMap.SampleLevel(ssPointWrap, float3(posX.x / nodeSize, 1.0 - posX.y / nodeSize, sliceIdx), 0).x * 255.0f;
    splatId.y = m_terrainSplatMap.SampleLevel(ssPointWrap, float3(posY.x / nodeSize, 1.0 - posY.y / nodeSize, sliceIdx), 0).x * 255.0f;
    splatId.z = m_terrainSplatMap.SampleLevel(ssPointWrap, float3(posZ.x / nodeSize, 1.0 - posZ.y / nodeSize, sliceIdx), 0).x * 255.0f;
    splatId.w = m_terrainSplatMap.SampleLevel(ssPointWrap, float3(posW.x / nodeSize, 1.0 - posW.y / nodeSize, sliceIdx), 0).x * 255.0f;
    float uvScale = 0.1f;
    float3 sampleX = float3(posUV * uvScale, splatId.x);
    float3 sampleY = float3(posUV * uvScale, splatId.y);
    float3 sampleZ = float3(posUV * uvScale, splatId.z);
    float3 sampleW = float3(posUV * uvScale, splatId.w);
    
    float4 norm0 = m_txTerrainNormalMap.SampleLevel(ssLinearWrap, sampleX, 0);
    float4 norm1 = m_txTerrainNormalMap.SampleLevel(ssLinearWrap, sampleY, 0);
    float4 norm2 = m_txTerrainNormalMap.SampleLevel(ssLinearWrap, sampleZ, 0);
    float4 norm3 = m_txTerrainNormalMap.SampleLevel(ssLinearWrap, sampleW, 0);
    float4 terrainNormalMap = norm0 * weight.x + norm1 * weight.y + norm2 * weight.z + norm3 * weight.w;

    float4 base0 = m_txTerrainBaseColor.SampleLevel(ssLinearWrap, sampleX, 0);
    float4 base1 = m_txTerrainBaseColor.SampleLevel(ssLinearWrap, sampleY, 0);
    float4 base2 = m_txTerrainBaseColor.SampleLevel(ssLinearWrap, sampleZ, 0);
    float4 base3 = m_txTerrainBaseColor.SampleLevel(ssLinearWrap, sampleW, 0);
    float4 terrainAlbedoMap = base0 * weight.x + base1 * weight.y + base2 * weight.z + base3 * weight.w;

    // 使用 physPageIdx 写入到正确的物理页位置
    m_txPhysicalPageAlbedo[uint3(dtid.xy, physPageIdx)] = terrainAlbedoMap;
    m_txPhysicalPageNormal[uint3(dtid.xy, physPageIdx)] = terrainNormalMap;
}
