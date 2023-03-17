#include "Common.fx"
#include "BRDF.fx"

#include "SHIrradianceCommon.fx"

struct ConstantBufferIrradSH
{
	float4 irradSH[9];
};


TextureCube txCubeMap : register(t0);
SamplerState ssLinearWrap : register(s0);

cbuffer ConstantBufferCubeMap : register(b1)
{
	float4 m_irradSH0123x;
	float4 m_irradSH4567x;
	float4 m_irradSH0123y;
	float4 m_irradSH4567y;
	float4 m_irradSH0123z;
	float4 m_irradSH4567z;
	float3 m_irradSH8xyz;
	float  m_cubeMapIntensity;
	float4 m_cubeMapIrradMode;
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

float4 GetSHIrradianceTest(float3 v)
{
	float3 irradiance = GetIrradiance(v, m_irradSH0123x, m_irradSH4567x, m_irradSH0123y, m_irradSH4567y, m_irradSH0123z, m_irradSH4567z, m_irradSH8xyz);
	return float4(irradiance, 1.0);
}

float4 PS(PS_INPUT input) : SV_Target
{
	bool bShowIrradianceOnly = m_cubeMapIrradMode.x > 0.5f;

	float4 intensity;
	if (bShowIrradianceOnly)
		intensity = GetSHIrradianceTest(input.posOS); 
	else
		intensity = txCubeMap.Sample(ssLinearWrap, input.posOS);

	intensity *= m_cubeMapIntensity;
	return intensity;
}
