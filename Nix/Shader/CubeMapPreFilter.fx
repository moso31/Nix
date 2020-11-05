#include "PBR.fx"
#include "Random.fx"

TextureCube txCubeMap : register(t0);

SamplerState samTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_projection;
}

cbuffer ConstantBufferPreFilter : register(b1)
{
	float m_roughness;
}

float3 GetPrefilter(float roughness, float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 result = 0;
	const uint NumSamples = 1024;
	float TotalWeight = 0.0f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(dot(N, L));
		if (NoL > 0.0f)
		{
			result += txCubeMap.SampleLevel(samTriLinearSam, L, 0).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return result / TotalWeight;
}

struct VS_INPUT
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float3 posL : POSITION;
	float3 normW : NORMAL;
	float2 tex : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posL = float4(input.pos, 1.0f);
	output.posH = mul(float4(input.pos, 1.0f), m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 0.0), m_worldInverseTranspose).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 irradiance = GetPrefilter(m_roughness, normalize(input.posL));
	return float4(irradiance, 1.0f);
}
