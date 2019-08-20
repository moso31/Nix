#include "Light.fx"

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBufferPrimitive : register(b0)
{
	matrix m_world;
	matrix m_worldInvTranspose;
}

cbuffer ConstantBufferCamera : register(b1)
{
	matrix m_view;
	matrix m_projection;
}

cbuffer ConstantBufferLight : register(b2)
{
	DirectionalLight m_dirLights[3];
}

cbuffer ConstantBufferMaterial : register(b3)
{
	Material m_material;
}

struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Norm : NORMAL0;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 PosW : POSITION;
	float3 NormW : NORMAL0;
	float2 Tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, m_world);
	output.Pos = mul(output.Pos, m_view);
	output.Pos = mul(output.Pos, m_projection);
	output.PosW = input.Pos;
	output.NormW = input.Norm;
	output.Tex = input.Tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return txDiffuse.Sample(samLinear, input.Tex);
}
