#include "Common.fx"
#include "Math.fx"
#include "PBRMaterials.fx"

Texture2D txAlbedo : register(t1);
Texture2D txNormalMap : register(t2);
Texture2D txMetallicMap : register(t3);
Texture2D txRoughnessMap : register(t4);
Texture2D txAmbientOcclusionMap : register(t5);

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
	float4 posOS : POSITION0;
	float4 posWS : POSITION1;
	float4 posVS : POSITION2;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

struct PS_OUTPUT
{
	float4 GBufferA : SV_Target0;
	float4 GBufferB : SV_Target1;
	float4 GBufferC : SV_Target2;
	float4 GBufferD : SV_Target3;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posOS = input.pos;
	output.posWS = mul(input.pos, m_world);
	output.posVS = mul(output.posWS, m_view);
	output.posSS = mul(output.posVS, m_projection);
	output.normVS = normalize(mul(input.norm, (float3x3)m_worldViewInverseTranspose));
	output.tex = input.tex;
	output.tangentVS = mul(input.tangent, (float3x3)m_worldViewInverseTranspose).xyz;
	return output;
}

void PS(PS_INPUT input, out PS_OUTPUT Output)
{
	Output.GBufferA = float4(input.posVS.xyz, 1.0f);

	float3 normalMap = txNormalMap.Sample(SamplerStateTrilinear, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	Output.GBufferB = float4(N, 1.0f);

	float3 albedoMap = txAlbedo.Sample(SamplerStateTrilinear, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;
	Output.GBufferC = float4(albedo, 1.0f);

	float metallicMap = txMetallicMap.Sample(SamplerStateTrilinear, input.tex).x;
	float metallic = m_material.metallic * metallicMap;

	float roughnessMap = txRoughnessMap.Sample(SamplerStateTrilinear, input.tex).x;
	float roughness = m_material.roughness * roughnessMap;

	float AOMap = txAmbientOcclusionMap.Sample(SamplerStateTrilinear, input.tex).x;
	float ao = m_material.ao * AOMap;

	Output.GBufferD = float4(roughness, metallic, ao, 1.0f);

	return;
}
