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
	float4 posSS : SV_POSITION;
	float4 posWS : POSITION0;
	float4 posVS : POSITION1;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posWS = mul(input.pos, m_world);
	output.posVS = mul(output.posWS, m_view);
	output.posSS = mul(output.posVS, m_projection);
	output.normVS = normalize(mul(input.norm, (float3x3)m_worldInverseTranspose));
	output.tex = input.tex;
	output.tangentVS = mul(input.tangent, (float3x3)m_world).xyz;
	return output;
}

float4 PS0(PS_INPUT input) : SV_Target
{
	float3 normalMap = txNormalMap.Sample(SamplerStateTrilinear, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	return float4(N, 1.0f);
}

float4 PS1(PS_INPUT input) : SV_Target
{
	return float4(input.posVS);
}