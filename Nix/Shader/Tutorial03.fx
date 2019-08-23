#include "Light.fx"

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBufferPrimitive : register(b0)
{
	matrix m_world;
}

cbuffer ConstantBufferCamera : register(b1)
{
	matrix m_view;
	matrix m_projection;
	float3 m_eyePos;
	float _align16;
}

cbuffer ConstantBufferLight : register(b2)
{
	DirectionalLight m_dirLight;
	PointLight m_pointLight;
	SpotLight m_spotLight;
}

cbuffer ConstantBufferMaterial : register(b3)
{
	Material m_material;
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
	float4 posW : POSITION;
	float3 normW : NORMAL0;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posW = output.posH;
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 1.0), m_world).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 toEye = normalize(m_eyePos - input.posW);
	float4 A, D, S, sumA, sumD, sumS;
	A = 0;
	D = 0;
	S = 0;
	sumA = 0;
	sumD = 0;
	sumS = 0;
	//ComputeDirectionalLight(m_material, m_dirLight, input.normW, toEye, A, D, S);
	sumA += A;
	sumD += D;
	sumS += S;
	//ComputePointLight(m_material, m_pointLight, input.posW, input.normW, toEye, A, D, S);
	sumA += A;
	sumD += D;
	sumS += S;
	ComputeSpotLight(m_material, m_spotLight, input.posW, input.normW, toEye, A, D, S);
	sumA += A;
	sumD += D;
	sumS += S;

	float4 result = sumA + sumD + sumS;
	return result;

	//return txDiffuse.Sample(samLinear, input.tex);
}
