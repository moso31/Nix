static const uint kMaxProcessingTilesAtOnce = 8;

struct CBufferVTConfig
{
	int2 TileSize;
	int BakeTileNum;
	int _0;
};

struct CBufferVTBatch
{
	int2 VTPageOffset;
	int2 TileWorldPos;
	int2 TileWorldSize;
	int2 _0;
};

cbuffer cbVTConfig : register(b0, space0)
{
	CBufferVTConfig m_cbVTConfig;
};

cbuffer cbVTBatch : register(b1, space0)
{
	CBufferVTBatch m_cbVTBatch[kMaxProcessingTilesAtOnce]; 
};

float4 RectGrid(float2 uv, out float2 vertexPosX, out float2 vertexPosY, out float2 vertexPosZ, out float2 vertexPosW)
{
    uv.y = 1.0f - uv.y;
    const float2 base = floor(uv + 0.5);
    vertexPosW = base + 0.5;
    vertexPosX = base - 0.5;
    vertexPosY = float2(vertexPosW.x - 1.0, vertexPosW.y);
    vertexPosZ = float2(vertexPosW.x, vertexPosW.y - 1.0);

    // Calculate offset from start texel to sample location
    float2 offset = uv + 0.5 - base;
    float2 oneMinusOffset = 1.0 - offset;

    // calculates the weights
    return float4(oneMinusOffset.x * oneMinusOffset.y, oneMinusOffset.x * offset.y, offset.x * oneMinusOffset.y, offset.x * offset.y);
}

Texture2DArray<float4> m_baseColorArray : register(t0, space0);
Texture2D m_batchTextures[] : register(t0, space1);

RWTexture2D<float> m_VTPhxPageSplatMap : register(u0, space0);
RWTexture2D<float> m_VTPhxPageHeightMap : register(u1, space0);

Texture2D<float> m_decalArray : register(t0, space2);

SamplerState ssPointWrap : register(s0);

// dtid: xy = 当前tile内的像素位置; z = 当前处理的 m_batchTextures[] 索引, t0, t2, t4 = heightMap; t1, t3, t5 = splatMap
[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID) 
{
    // 像素的世界坐标
    float2 worldPixelXZ = m_cbVTBatch[dtid.z].TileWorldPos + (float2)dtid.xy / m_cbVTConfig.TileSize * (float2)m_cbVTBatch[dtid.z].TileWorldSize;

    // HeightMap Or SplatMaps
    float2 heightMapUV = (float2)dtid.xy / m_cbVTConfig.TileSize;
    float2 splatMapUV = (float2)dtid.xy / m_cbVTConfig.TileSize;
    float heightMapData = m_batchTextures[dtid.z * 2 + 0].SampleLevel(ssPointWrap, heightMapUV, 0.0f).x * 255.0f;
    float splatMapData = m_batchTextures[dtid.z * 2 + 1].SampleLevel(ssPointWrap, splatMapUV, 0.0f).x * 255.0f;

    int2 pixelPos = m_cbVTBatch[dtid.z].VTPageOffset;
    m_VTPhxPageHeightMap[pixelPos + dtid.xy] = heightMapData;
    m_VTPhxPageSplatMap[pixelPos + dtid.xy] = splatMapData;
}
