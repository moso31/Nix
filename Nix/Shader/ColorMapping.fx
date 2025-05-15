Texture2D txRenderTarget : register(t0);
SamplerState samLinear : register(s0);

cbuffer CBufferParams : register(b2)
{
	float4 param0; // x : enable;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;

	return output;
}

float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 LinearToSRGB(float3 c)
{
	float gamma = 1.0 / 2.4;
	float3 sRGBLo = c * 12.92;
	float3 sRGBHi = (max(pow(c, float3(gamma, gamma, gamma)), 0.0) * 1.055) - 0.055;
	float3 sRGB = select(sRGBLo, sRGBHi, c > 0.0031308);
	return sRGB;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 color = txRenderTarget.Sample(samLinear, input.tex).xyz;

	if (param0.x < 0.5f)
		return float4(color, 1.0f);

	color = ACESFilm(color);

	color = LinearToSRGB(color);

	return float4(color, 1.0f);
}
