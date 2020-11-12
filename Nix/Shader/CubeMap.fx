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

struct VS_INPUT
{
	float3 pos : POSITION;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float3 posL : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(float4(input.pos, 1.0f), m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection).xyww;
	output.posL = input.pos;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 result = txCubeMap.Sample(samTriLinearSam, input.posL);
	return result;
}
