#ifndef TILE_SIZE
#define TILE_SIZE 8
#endif

cbuffer CBufferVTReadback : register(b0)
{
	float4 m_param0; // xy : rtSize; zw : dither8x8
};

Texture2D<float> txVTEncodeData : register(t0);
RWStructuredBuffer<uint> m_output : register(u0);

// ��������߳��������Ѿ���1/8 RT������������
[numthreads(1, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
	float2 screenPos = (float2)(DTid.xy * TILE_SIZE) + m_param0.zw;
	if (screenPos.x >= m_param0.x || screenPos.y >= m_param0.y)
		return;

	float2 outPos = (float2)DTid.xy;
	float2 outRTSize = ceil(m_param0.xy / TILE_SIZE);
	if (outPos.x >= outRTSize.x || outPos.y >= outRTSize.y)
		return;

	float encode = txVTEncodeData.Load(int3(screenPos, 0)).x;
	int index = outPos.y * m_param0.x + outPos.x;
	m_output[index] = asuint(encode);
}
