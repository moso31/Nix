Texture2D txIrradiance : register(t0);
Texture2D txSpecular : register(t1);
SamplerState ssLinearClamp : register(s0);

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

float4 PS(PS_INPUT input) : SV_Target
{
	float3 irradiance = txIrradiance.Sample(ssLinearClamp, input.tex).xyz;
	float3 spec = txSpecular.Sample(ssLinearClamp, input.tex).xyz;
	float3 result = irradiance + spec;
	return float4(result, 1.0f);
}
