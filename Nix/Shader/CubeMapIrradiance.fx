#include "PBR.fx"

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

float3 GetIrradianceMap(float3 wiWorld)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 right = cross(wiWorld, up);
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
	output.posL = input.pos, 1.0f;
	output.posH = mul(float4(input.pos, 1.0f), m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 0.0), m_worldInverseTranspose).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 irradiance = GetIrradianceMap(input.posL);
	return txCubeMap.Sample(samTriLinearSam, input.posL).yzxw;
}
