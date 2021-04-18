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
	float Normal = txNormal.Load(DTid).z;
	txSimpleSSAO[DTid.xy] = LinearDepth(DepthZ);
}