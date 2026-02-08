#include "Common.fx"
#include "Math.fx"

Texture2D txLoading : register(t1);
SamplerState ssLinearWrap : register(s0);

struct TriplanarUV
{
	float2 x, y, z;
};

TriplanarUV GetTriplanarUV(float3 position)
{
	TriplanarUV triUV;
	float3 p = -position * 0.25f;
	triUV.x = p.zy;
	triUV.y = p.xz;
	triUV.z = p.xy;
	triUV.x *= float2(-1.0f, 1.0f);
	triUV.y *= float2(-1.0f, 1.0f);
	triUV.z *= float2(-1.0f, 1.0f);
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
	float3 normWS : NORMAL1;
	float2 tex : TEXCOORD;
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
	output.normWS = mul(output.normOS, (float3x3)m_worldInverseTranspose);
	output.tex = input.tex;
	return output;
}

void PS(PS_INPUT input, out PS_OUTPUT output)
{
	float3 posWS = input.posWS.xyz;
	TriplanarUV triUV = GetTriplanarUV(posWS);

	float3 albedoX = txLoading.Sample(ssLinearWrap, triUV.x).xyz;
	float3 albedoY = txLoading.Sample(ssLinearWrap, triUV.y).xyz;
	float3 albedoZ = txLoading.Sample(ssLinearWrap, triUV.z).xyz;

	float3 weights = abs(normalize(input.normWS));
	weights = pow(weights, 8.0f);
	weights /= dot(weights, 1.0.xxx);

	float3 albedo = weights.x * albedoX + weights.y * albedoY + weights.z * albedoZ;

	// TODO: 换成VirtPageID，临时代码
	int ix = (int)floor(input.posWS.x);
	int iy = (int)floor(input.posWS.y);
	int iz = (int)floor(input.posWS.z);
	uint ux = (ix % 4096u) & 0xFFFu;
	uint uy = (iy % 4096u) & 0xFFFu;
	uint uz = (iz % 256u) & 0xFFu;
	uint packed = (ux << 20) | (uy << 8) | uz;

	output.GBufferA = asfloat(packed);
	output.GBufferB = float4(0.5f, 0.5f, 1.0f, 1.0f); // xyz = vector(0, 0, 1)
	output.GBufferC = float4(albedo, 1.0f); // use txLoading as albedo.
	output.GBufferD = float4(0.0f, 1.0f, 1.0f, 1.0f / 255.0f); // xyz = Metallic/Roughness/AO, w = shading model 'unlit'.
}
