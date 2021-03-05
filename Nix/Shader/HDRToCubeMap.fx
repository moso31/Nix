Texture2D txHDRMap : register(t0);

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
	matrix m_worldViewInverseTranspose;
	matrix m_projection;
}

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
	float4 posH : SV_POSITION;
	float3 posL : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posL = float4(input.pos, 1.0f);
	output.posH = mul(float4(input.pos, 1.0f), m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = SampleSphericalMap(normalize(input.posL)); // make sure to normalize localPos
	float3 color = txHDRMap.Sample(samTriLinearSam, uv).rgb;
	return float4(color, 1.0);
}
