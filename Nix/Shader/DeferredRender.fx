#include "Common.fx"
#include "PBRLights.fx"
#include "BRDF.fx"
#include "Math.fx"
#include "SHIrradianceCommon.fx"

Texture2D txRT0 : register(t0);
Texture2D txRT1 : register(t1);
Texture2D txRT2 : register(t2);
Texture2D txRT3 : register(t3);
Texture2D txRTDepth : register(t4);
TextureCube txCubeMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);
Texture2D txShadowTest : register(t8);

SamplerState ssLinearWrap : register(s0);
SamplerState ssLinearClamp : register(s1);

cbuffer ConstantBufferCamera : register(b1)
{
	float4 cameraParams0;
	float4 cameraParams1; // n, f, f / (f - n), -f * n / (f - n)
	float4 cameraParams2;
}

cbuffer ConstantBufferLight : register(b2)
{
	DistantLight m_dirLight[NUM_DISTANT_LIGHT];
	PointLight m_pointLight[NUM_POINT_LIGHT];
	SpotLight m_spotLight[NUM_SPOT_LIGHT];
}

cbuffer ConstantBufferCubeMap : register(b3)
{
	float4 m_irradSH0123x;
	float4 m_irradSH4567x;
	float4 m_irradSH0123y;
	float4 m_irradSH4567y;
	float4 m_irradSH0123z;
	float4 m_irradSH4567z;
	float3 m_irradSH8xyz;
	float  m_cubeMapIntensity;
	float4 m_cubeMapIrradMode;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float2 tex : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;
	return output;
}

float DepthZ01ToLinear(float z01)
{
	float A = cameraParams1.z;
	float B = cameraParams1.w;
	return B / z01 - A;
}

float3 CalcBSDF(float NoV, float NoL, float NoH, float VoH, float roughness, float metallic, float3 albedo, float3 F0)
{
	float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);

	// 微表面 BRDF
	float D = D_GGX(NoH, roughness);
	float G = G_GGX_SmithJoint(NoV, NoL, roughness);
	float3 F = F_Schlick(saturate(VoH), F0);
	float3 specular = D * G * F;

	return diffuse + specular;
}

float3 GetIndirectIrradiance(float3 v)
{
	float3 irradiance = GetIrradiance(v, m_irradSH0123x, m_irradSH4567x, m_irradSH0123y, m_irradSH4567y, m_irradSH0123z, m_irradSH4567z, m_irradSH8xyz);
	return irradiance;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = input.tex;
	float a = txRT3.Sample(ssLinearWrap, uv).z;
	//return float4(a.xxx, 1.0f);
	float3 PositionVS = txRT0.Sample(ssLinearWrap, uv).xyz;
	float3 N = txRT1.Sample(ssLinearWrap, uv).xyz;
	float3 V = normalize(-PositionVS);
	float NoV = max(dot(N, V), 0.0);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	// get depthZ
	float depth = txRTDepth.Sample(ssLinearClamp, uv).x;
	// convert depth to linear
	float linearDepthZ = DepthZ01ToLinear(depth);
	return float4(linearDepthZ.xxxx);

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedo = txRT2.Sample(ssLinearWrap, uv).xyz;

	float roughnessMap = txRT3.Sample(ssLinearWrap, uv).x;
	float roughness = roughnessMap;
	float perceptualRoughness = roughness * roughness;

	float metallicMap = txRT3.Sample(ssLinearWrap, uv).y;
	float metallic = metallicMap;

	float AOMap = txRT3.Sample(ssLinearWrap, uv).z;
	float ao = AOMap;

	float3 ShadowTest = txShadowTest.Sample(ssLinearClamp, uv).xyz;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);
	albedo *= 1.0f - metallic;

	float3 Lo = 0.0f;
	int i;
	for (i = 0; i < NUM_DISTANT_LIGHT; i++)
	{
		float3 LightDirWS = normalize(-m_dirLight[i].direction);
		float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_viewInverseTranspose));

		float3 L = LightDirVS;
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float3 LightIlluminance = m_dirLight[i].color * m_dirLight[i].illuminance; // 方向光的Illuminace
		float3 IncidentIlluminance = LightIlluminance * NoL;

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance; // Output radiance.
	}

    for (i = 0; i < NUM_POINT_LIGHT; i++)
    {
		float3 LightPosVS = mul(float4(m_pointLight[i].position, 1.0f), m_view).xyz;
		float3 LightIntensity = m_pointLight[i].color * m_pointLight[i].intensity;
		float3 LightDirVS = LightPosVS - PositionVS;

        float3 L = normalize(LightDirVS);
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float d2 = dot(LightDirVS, LightDirVS);
		float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
		float3 IncidentIlluminance = LightIlluminance * NoL;

		float Factor = d2 / (m_pointLight[i].influenceRadius * m_pointLight[i].influenceRadius);
		float FalloffFactor = max(1.0f - Factor * Factor, 0.0f);

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance * FalloffFactor; // Output radiance.
    }

	for (i = 0; i < NUM_SPOT_LIGHT; i++)
	{
		float3 LightPosVS = mul(float4(m_spotLight[i].position, 1.0f), m_view).xyz;
		float3 LightIntensity = m_spotLight[i].color * m_spotLight[i].intensity;
		float3 LightDirVS = LightPosVS - PositionVS;

		float3 L = normalize(LightDirVS);
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float d2 = dot(LightDirVS, LightDirVS);
		float3 LightIlluminance = LightIntensity / (NX_PI * d2);
		float3 IncidentIlluminance = LightIlluminance * NoL;

		float CosInner = cos(m_spotLight[i].innerAngle * NX_DEGTORED);
		float CosOuter = cos(m_spotLight[i].outerAngle * NX_DEGTORED);
		float3 SpotDirWS = normalize(m_spotLight[i].direction);
		float3 SpotDirVS = normalize(mul(SpotDirWS, (float3x3)m_viewInverseTranspose));
		float FalloffFactor = (dot(-SpotDirVS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
		FalloffFactor = saturate(FalloffFactor);
		FalloffFactor = FalloffFactor * FalloffFactor;

		float Factor = d2 / (m_spotLight[i].influenceRadius * m_spotLight[i].influenceRadius);
		FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance * FalloffFactor; // Output radiance.
	}

	float3 NormalWS = mul(N, (float3x3)m_viewTranspose);
	//float3 IndirectIrradiance = txIrradianceMap.Sample(ssLinearWrap, NormalWS).xyz;
	float3 IndirectIrradiance = GetIndirectIrradiance(NormalWS);
	float3 diffuseIBL = albedo * IndirectIrradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, perceptualRoughness * 4.0f).rgb; // 4.0 = prefilter mip count - 1.
	float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
	float3 SpecularIBL = preFilteredColor * lerp(envBRDF.xxx, envBRDF.yyy, F0);

	float3 energyCompensation = 1.0f + F0 * (1.0f / envBRDF.yyy - 1.0f);
	SpecularIBL *= energyCompensation;

	float3 Libl = (diffuseIBL + SpecularIBL) * m_cubeMapIntensity * ao;
	float3 color = Libl + Lo * (1.0f - ShadowTest);

	return float4(color, 1.0f);
}
