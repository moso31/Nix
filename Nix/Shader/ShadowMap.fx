cbuffer ConstantBufferPrimitive : register(b0)
{
	matrix m_world;
}

cbuffer ConstantBufferShadowMapCamera : register(b1)
{
	matrix m_view;
	matrix m_projection;
	matrix m_tex;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float3 normW : NORMAL0;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 1.0), m_world).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
