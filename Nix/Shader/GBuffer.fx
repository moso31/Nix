#include "Math.fx"
#include "PBRMaterials.fx"

SamplerState samTriLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D txAlbedo : register(t1);
Texture2D txNormalMap : register(t2);
Texture2D txMetallicMap : register(t3);
Texture2D txRoughnessMap : register(t4);
Texture2D txAmbientOcclusionMap : register(t5);

cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_projection;
}

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
	float4 posL : POSITION0;
	float4 posW : POSITION1;
	float4 posV : POSITION2;
	float3 normW : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentW : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posL = input.pos;
	output.posH = mul(input.pos, m_world);
	output.posW = output.posH;
	output.posH = mul(output.posH, m_view);
	output.posV = output.posH;
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 0.0), m_worldInverseTranspose).xyz;
	output.tex = input.tex;
	output.tangentW = mul(input.tangent, (float3x3)m_world).xyz;
	return output;
}

float4 PS_RT0(PS_INPUT input) : SV_Target
{
	return float4(input.posV.zzz, 1.0f);
	return float4(input.posW.xyz, 1.0f);
}

float4 PS_RT1(PS_INPUT input) : SV_Target
{
	float3 normalMap = txNormalMap.Sample(samTriLinear, input.tex).xyz;
	float3 N = TangentSpaceToWorldSpace(normalMap, input.normW, input.tangentW, input.tex);
	return float4(N, 1.0f);
}

float4 PS_RT2(PS_INPUT input) : SV_Target
{
	float3 albedoMap = txAlbedo.Sample(samTriLinear, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;
	return float4(albedo, 1.0f);
}

float4 PS_RT3(PS_INPUT input) : SV_Target
{
	float roughnessMap = txRoughnessMap.Sample(samTriLinear, input.tex).x;
	//float roughness = m_material.roughness;
	float roughness = roughnessMap;

	float metallicMap = txMetallicMap.Sample(samTriLinear, input.tex).x;
	//float metallic = m_material.metallic;
	float metallic = metallicMap;

	float AOMap = txAmbientOcclusionMap.Sample(samTriLinear, input.tex).x;
	//float metallic = m_material.metallic;
	float ao = AOMap;

	return float4(roughness, metallic, ao, 1.0f);
}