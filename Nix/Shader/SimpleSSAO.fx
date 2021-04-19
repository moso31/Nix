#include "Math.fx"

SamplerState SamplerStateTrilinear : register(s0);

cbuffer ConstantBufferCamera : register(b0)
{
	float3 m_cameraPosition;
	float _0;
	float4 zBufferParam;
}

Texture2D txNormal : register(t0);
Texture2D txDepthZ : register(t1);

RWTexture2D<float> txSimpleSSAO : register(u0);

float LinearDepth(float z)
{
	return 1.0f / (zBufferParam.z * z + zBufferParam.w);
}

[numthreads(8, 8, 1)]
void CS(int3 DTid : SV_DispatchThreadID)
{
	float DepthZ = txDepthZ.Load(DTid).x;

	// N为Z轴，TB随意的xyz坐标基
	float3 N = txNormal.Load(DTid);
	float3 T = cross(N, float3(1.0f, 0.0f, 0.0f));
	if (!any(T))
		T = cross(N, float3(0.0f, 1.0f, 0.0f));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);



	txSimpleSSAO[DTid.xy] = InterleavedGradientNoise(DTid.xy);
	//LinearDepth(DepthZ);
}