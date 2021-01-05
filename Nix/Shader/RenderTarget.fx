Texture2D txRenderTarget : register(t0);
SamplerState samLinear : register(s0);

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
	output.posH = input.pos;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 result = txRenderTarget.Sample(samLinear, input.tex);
	return result;
}
