cbuffer ConstantBufferShadowMapObject : register(b0)
{
	matrix m_world;
	matrix m_view;
	matrix m_projection;
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
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
