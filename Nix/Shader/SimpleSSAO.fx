#include "Random.fx"

#define SSAO_SAMPLE_COUNT 64

SamplerState SamplerStateTrilinear : register(s0);

//TODO:命名修改，所有ConstantBufferCamera改成cbCamera
cbuffer ConstantBufferCamera : register(b0)
{
	float4 m_cameraPosition;
	float4 m_zBufferParam;
	float4 m_projParams;
}

Texture2D txNormal : register(t0);
Texture2D txDepthZ : register(t1);

RWTexture2D<float> txSimpleSSAO : register(u0);

float LinearDepth(float z)
{
	return 1.0f / (m_zBufferParam.z * z + m_zBufferParam.w);
}

[numthreads(8, 8, 1)]
void CS(int3 DTid : SV_DispatchThreadID)
{
	// N为Z轴，TB随意的xyz坐标基
	float3 N = txNormal.Load(DTid).xyz;
	float3 T = cross(N, float3(1.0f, 0.0f, 0.0f));
	if (!any(T))
		T = cross(N, float3(0.0f, 1.0f, 0.0f));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);

	float sumWeight = 0.0f;
	for (int i = 0; i < SSAO_SAMPLE_COUNT; i++)
	{
		float3 SamplePos = normalize(float3(
			InterleavedGradientNoise(DTid.xy, i * 3 + 0) * 2.0f - 1.0f,
			InterleavedGradientNoise(DTid.xy, i * 3 + 1) * 2.0f - 1.0f,
			InterleavedGradientNoise(DTid.xy, i * 3 + 2)
		));

		float3 SampleVS = mul(SamplePos, TBN);
		float2 SampleNDC = SampleVS.xy * m_projParams.xy;
		float2 SampleUV = (SampleNDC + 1.0f) * 0.5f;

		float SampleDepth = txDepthZ.SampleLevel(SamplerStateTrilinear, SampleUV, 0.0).x;
		float SampleLinearDepth = LinearDepth(SampleDepth);
		if (SampleVS.z > SampleLinearDepth)
		{
			sumWeight += 1.0f;
		}
	}

	txSimpleSSAO[DTid.xy] = sumWeight / (float)SSAO_SAMPLE_COUNT;
}