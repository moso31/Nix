#include "Common.fx"

struct VS_INPUT
{
	float4 pos : POSITION;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 posW : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posW = output.posH;
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return input.posW;
}