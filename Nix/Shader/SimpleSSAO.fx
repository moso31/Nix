#include "Common.fx"

struct VS_INPUT
{
	float4 pos : POSITION;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float4 posWS : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = mul(input.pos, m_world);
	output.posWS = output.posSS;
	output.posSS = mul(output.posSS, m_view);
	output.posSS = mul(output.posSS, m_projection);
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return input.posWS;
}