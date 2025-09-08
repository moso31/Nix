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

Texture2D<float4> m_batchTextures[] : register(t0, space1);

RWTexture2D<float3> m_VTPhxPageSplatMap : register(u0, space0);
RWTexture2D<float> m_VTPhxPageHeightMap : register(u1, space0);

Texture2D<float> m_decalArray : register(t0, space1);

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
	int2 pixelPos = m_cbVTBatch[dtid.z].VTPageOffset;
	float tempVal = (float)pixelPos / 8192.0f;
	m_VTPhxPageSplatMap[pixelPos + dtid.xy] = float3(0.0f, tempVal.x, 0.0f);
	m_VTPhxPageHeightMap[pixelPos + dtid.xy] = tempVal;
}