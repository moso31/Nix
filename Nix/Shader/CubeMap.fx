#include "Common.fx"
#include "BRDF.fx"

TextureCube txCubeMap : register(t0);

cbuffer ConstantBufferCubeMap : register(b1)
{
	float m_cubeMapIntensity;
	float3 _0;
}

struct VS_INPUT
{
	float3 pos : POSITION;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float3 posOS : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = mul(float4(input.pos, 1.0f), m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posSS = mul(output.posSS, m_projection).xyww;
	output.posOS = input.pos;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 intensity = txCubeMap.Sample(SamplerStateTrilinear, input.posOS);
	intensity *= m_cubeMapIntensity;
	return pow(intensity, 0.45454545454545f);
}
