#include "common.fx"

// shadow map 使用独立的 viewproj 矩阵
cbuffer ConstantBufferShadowMap : register(b2)
{
	matrix m_csmView;
	matrix m_csmProj;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posH = mul(output.posH, m_csmView);
	output.posH = mul(output.posH, m_csmProj);
	output.tex = input.tex;

	return output;
}

void PS(PS_INPUT input)
{
}
