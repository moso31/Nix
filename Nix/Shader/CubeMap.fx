#include "Common.fx"
#include "BRDF.fx"

#include "SHIrradianceCommon.fx"

struct ConstantBufferIrradSH
{
	float4 irradSH[9];
};


TextureCube txCubeMap : register(t0);
StructuredBuffer<ConstantBufferIrradSH> cbIrradianceSH : register(t1);
TextureCube txIrradianceMap : register(t2);

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
	float4 intensity = 0.0f;

	if (m_cubeMapIrradMode.x < 2.5f)
	{
		intensity.x =
			g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].x +
			g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].x * v.x +
			g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].x * v.y +
			g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].x * v.z +
			g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].x * v.x * v.z +
			g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].x * v.x * v.y +
			g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].x * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
			g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].x * v.y * v.z +
			g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].x * (v.z * v.z - v.x * v.x);

		intensity.y =
			g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].y +
			g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].y * v.x +
			g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].y * v.y +
			g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].y * v.z +
			g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].y * v.x * v.z +
			g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].y * v.x * v.y +
			g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].y * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
			g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].y * v.y * v.z +
			g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].y * (v.z * v.z - v.x * v.x);

		intensity.z =
			g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].z +
			g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].z * v.x +
			g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].z * v.y +
			g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].z * v.z +
			g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].z * v.x * v.z +
			g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].z * v.x * v.y +
			g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
			g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].z * v.y * v.z +
			g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].z * (v.z * v.z - v.x * v.x);
	}
	else
	{
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
	}

	intensity.w = 1.0f;

	return intensity;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 intensity;

	if (m_cubeMapIrradMode.x < 0.5f)
		intensity = txCubeMap.Sample(ssLinearWrap, input.posOS);

	else if (m_cubeMapIrradMode.x < 1.5f)
		intensity = txIrradianceMap.Sample(ssLinearWrap, input.posOS);

	// test: Show SH irradiance only
	else 
		intensity = GetSHIrradianceTest(input.posOS); 

	intensity *= m_cubeMapIntensity;
	return pow(intensity, 1.0f);
	//return pow(intensity, 0.45454545454545f);
}
