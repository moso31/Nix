#include "Common.fx"

Texture2D txHDRMap : register(t0);

SamplerState ssLinearWrap : register(s0);

static const float2 invAtan = float2(0.1591f, 0.3183f);
float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5f;
	return float2(uv.x, 1.0f - uv.y);
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
	output.posOS = float4(input.pos, 1.0f);
	output.posSS = mul(float4(input.pos, 1.0f), m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posSS = mul(output.posSS, m_projection);

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = SampleSphericalMap(normalize(input.posOS)); // make sure to normalize localPos
	float3 color = txHDRMap.Sample(ssLinearWrap, uv).rgb;
	return float4(color, 1.0);
}
