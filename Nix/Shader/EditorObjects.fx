#include "Common.fx"

cbuffer CBufferParams : register(b2)
{
	float4 param0;
}

#define EditorObject_IsHighLight param0.x

struct VS_INPUT
{
	float4 pos : POSITION;
	float4 color : COLOR0;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float4 posOS : POSITION0;
	float4 posWS : POSITION1;
	float4 posVS : POSITION2;
	float4 color : COLOR0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posOS = input.pos;
	output.posWS = mul(input.pos, m_world);
	output.posVS = mul(output.posWS, m_view);
	output.posSS = mul(output.posVS, m_projection);
	output.color = input.color;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	if (EditorObject_IsHighLight) return float4(1.0f, 1.0f, 0.3f, 0.8f);
	return input.color;
}
