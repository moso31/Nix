#include "Common.fx"
#include "BRDF.fx"

#include "SphereHarmonic.fx"

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
	float4 wtf = txCubeMap.Sample(ssLinearWrap, input.posOS);

	float3 v = normalize(input.posOS); // view direction
	float4 intensity = 0.0f;
	intensity.x = 
		g_SHFactor[0] * m_irradSH0123x.x +
		g_SHFactor[1] * m_irradSH0123x.y * v.x +
		g_SHFactor[2] * m_irradSH0123x.z * v.y +
		g_SHFactor[3] * m_irradSH0123x.w * v.z +
		g_SHFactor[4] * m_irradSH4567x.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567x.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567x.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567x.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.x * (v.z * v.z - v.x * v.x);

	intensity.y = 
		g_SHFactor[0] * m_irradSH0123y.x +
		g_SHFactor[1] * m_irradSH0123y.y * v.x +
		g_SHFactor[2] * m_irradSH0123y.z * v.y +
		g_SHFactor[3] * m_irradSH0123y.w * v.z +
		g_SHFactor[4] * m_irradSH4567y.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567y.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567y.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567y.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.y * (v.z * v.z - v.x * v.x);

	intensity.z =
		g_SHFactor[0] * m_irradSH0123z.x +
		g_SHFactor[1] * m_irradSH0123z.y * v.x +
		g_SHFactor[2] * m_irradSH0123z.z * v.y +
		g_SHFactor[3] * m_irradSH0123z.w * v.z +
		g_SHFactor[4] * m_irradSH4567z.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567z.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567z.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567z.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.z * (v.z * v.z - v.x * v.x);

	intensity.w = 1.0f;

	//intensity *= 0.5f;
	//intensity *= m_cubeMapIntensity;
	intensity = lerp(wtf, intensity, m_cubeMapIntensity * 0.1f);
	return pow(intensity, 1.0f);
	return pow(intensity, 0.45454545454545f);
}
