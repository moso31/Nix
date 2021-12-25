#include "Common.fx"
#include "Math.fx"
#include "PBRMaterials.fx"

Texture2D txNormalMap : register(t0);

cbuffer ConstantBufferMaterial : register(b2)
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

struct PS_OUTPUT
{
	float4 Normal : SV_Target0;
	float4 Position: SV_Target1;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
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
	Output.Position = float4(input.posVS);

	float3 normalMap = txNormalMap.Sample(SamplerStateTrilinear, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	Output.Normal = float4(N, 1.0f);
}
