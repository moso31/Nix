#include "Common.fx"
#include "Math.fx"

struct CBufferLoading
{
	float2 uvScale;
	float2 _0;
};

Texture2D txLoading : register(t1);
SamplerState ssLinearWrap : register(s0);

struct TriplanarUV
{
	float2 x, y, z;
};

cbuffer CBuffer : register(b3)
{
	CBufferLoading m_material;
}

TriplanarUV GetTriplanarUV(float3 position)
{
	TriplanarUV triUV;
	float3 p = position;
	triUV.x = p.zy;
	triUV.y = p.xz;
	triUV.z = p.xy;
	return triUV;
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
	float3 normOS : NORMAL0;
	float3 normVS : NORMAL1;
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
	output.normOS = normalize(input.norm);
	output.normVS = mul(output.normOS, (float3x3)m_worldViewInverseTranspose);
	output.tex = input.tex;
	output.tangentVS = normalize(mul(input.tangent, (float3x3)m_worldViewInverseTranspose));
	return output;
}

void PS(PS_INPUT input, out PS_OUTPUT output)
{
	float3 posOS = input.posOS.xyz;
	TriplanarUV triUV = GetTriplanarUV(posOS);

	float3 albedoX = txLoading.Sample(ssLinearWrap, triUV.x).xyz;
	float3 albedoY = txLoading.Sample(ssLinearWrap, triUV.y).xyz;
	float3 albedoZ = txLoading.Sample(ssLinearWrap, triUV.z).xyz;
	float3 albedo = (albedoX + albedoY + albedoZ) / 3;

	output.GBufferA = float4(input.posVS.xyz, 1.0f);
	output.GBufferB = float4(0.5f, 0.5f, 1.0f, 1.0f); // xyz = vector(0, 0, 1)
	output.GBufferC = float4(albedo, 1.0f); // use txLoading as albedo.
	output.GBufferD = float4(0.0f, 1.0f, 1.0f, 1.0f);
}
