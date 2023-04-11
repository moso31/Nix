#include "Common.fx"
#include "Math.fx"
#include "PBRMaterials.fx"

Texture2D txAlbedo : register(t1);
Texture2D txNormalMap : register(t2);
Texture2D txMetallicMap : register(t3);
Texture2D txRoughnessMap : register(t4);
Texture2D txAmbientOcclusionMap : register(t5);

SamplerState ssLinearWrap : register(s0);

cbuffer CBufferMaterialStandard : register(b3)
{
	PBRMaterialStandard m_material;
}

struct PSInputData
{
	float3 positionVS;
	float3 normalVS;
	float2 texcoord;
	float3 tangentVS;
};

struct PSOutputData
{
	float3 position;
	float3 normal;
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
};

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
	output.tangentVS = normalize(mul(input.tangent, (float3x3)m_worldViewInverseTranspose));
	return output;
}

void BuildPixelShaderInputData(PS_INPUT input, out PSInputData data)
{
	data.positionVS = input.posVS.xyz;
	data.normalVS = input.normVS;
	data.texcoord = input.tex;
	data.tangentVS = input.tangentVS;
}

PSOutputData EvaluateMaterial(PSInputData input)
{
	PSOutputData result;
	result.position = input.positionVS;
	result.normal = input.normalVS;
	result.albedo = txAlbedo.Sample(ssLinearWrap, input.texcoord).rgb;
	result.metallic = txMetallicMap.Sample(ssLinearWrap, input.texcoord).r;
	result.roughness = txRoughnessMap.Sample(ssLinearWrap, input.texcoord).r;
	result.ao = txAmbientOcclusionMap.Sample(ssLinearWrap, input.texcoord).r;
}

void EncodeGBuffer(float3 Position, float3 normal, float3 albedo, float metallic, float roughness, float ao, out PS_OUTPUT output)
{
	output.GBufferA = float4(Position, 1.0f);
	output.GBufferB = float4(normal, 1.0f);
	output.GBufferC = float4(albedo, 1.0f);
	output.GBufferD = float4(metallic, roughness, ao, 1.0f);
}

void PS(PS_INPUT input, out PS_OUTPUT Output)
{
	PSInputData psDataIn;
	BuildPixelShaderInputData(input, psDataIn);
	//psDataOut = EvaluateMaterial(psDataIn); // 这句话每个材质都不一样

	EncodeGBuffer(position, normal, albedo, metallic, roughness, ao);
	return;

	Output.GBufferA = float4(input.posVS.xyz, 1.0f);

	float3 normalMap = txNormalMap.Sample(ssLinearWrap, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	Output.GBufferB = float4(N, 1.0f);

	float3 albedoMap = txAlbedo.Sample(ssLinearWrap, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;
	Output.GBufferC = float4(albedo, 1.0f);

	float metallicMap = txMetallicMap.Sample(ssLinearWrap, input.tex).x;
	float metallic = m_material.metallic * metallicMap;

	float roughnessMap = txRoughnessMap.Sample(ssLinearWrap, input.tex).x;
	float roughness = m_material.roughness * roughnessMap;

	float AOMap = txAmbientOcclusionMap.Sample(ssLinearWrap, input.tex).x;
	float ao = m_material.ao * AOMap;

	Output.GBufferD = float4(roughness, metallic, ao, 1.0f);

	return;
}
