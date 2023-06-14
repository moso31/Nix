#include "Common.fx"
#include "Math.fx"

struct CBufferLoading
{
	float2 uvScale;
	float2 uvOffset;
};

SamplerState ssLinearWrap : register(s0);

Texture2D tx[10][10] : register(t1);

cbuffer CBuffer : register(b3)
{
	CBufferLoading m_material;
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
	float3 albedo = float3(0.0f, 0.0f, 0.0f);

	float2 uvScale = 1.0f;
	for (float i = 0.0f; i < 9.9f; i += 1.0f)
	{
		for (float j = 0.0f; j < 9.9f; j += 1.0f)
		{
			float2 uvOffset = float2(i, j) * 0.1f;
			float2 uv = input.tex * uvScale + uvOffset;
			albedo += tx[i][j].Sample(ssLinearWrap, uv).xyz * 0.01;
		}
	}

	output.GBufferA = float4(input.posVS.xyz, 1.0f);
	output.GBufferB = float4(0.5f, 0.5f, 1.0f, 1.0f); // xyz = vector(0, 0, 1)
	output.GBufferC = float4(albedo, 1.0f); // use txLoading as albedo.
	output.GBufferD = float4(1.0f, 0.0f, 1.0f, 1.0f);
}
