#include "Common.fx"
#include "Math.fx"
#include "PBRMaterials.fx"

Texture2D txNormalMap : register(t0);

cbuffer ConstantBufferMaterial : register(b3)
{
	Material m_material;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangent : TANGENT;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 posW : POSITION;
	float3 normW : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentW : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posW = output.posH;
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = normalize(mul(input.norm, (float3x3)m_worldInverseTranspose));
	output.tex = input.tex;
	output.tangentW = mul(input.tangent, (float3x3)m_world).xyz;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 normalMap = txNormalMap.Sample(SamplerStateTrilinear, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;
	float3 N = TangentSpaceToWorldSpace(normal, input.normW, input.tangentW, input.tex);
	return float4(N, 1.0f);
}