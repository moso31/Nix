#include "Common.fx"
#include "PBRLights.fx"
#include "BRDF.fx"
#include "Math.fx"
#include "SphereHarmonic.fx"

Texture2D txRT0 : register(t0);
Texture2D txRT1 : register(t1);
Texture2D txRT2 : register(t2);
Texture2D txRT3 : register(t3);
TextureCube txCubeMap : register(t4);
TextureCube txIrradianceMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);
Texture2D txSSAO : register(t8);

struct ConstantBufferIrradSH
{
	float4 irradSH[9];
};

StructuredBuffer<ConstantBufferIrradSH> cbIrradianceSH : register(t9);

SamplerState ssLinearWrap : register(s0);
SamplerState ssLinearClamp : register(s1);

cbuffer ConstantBufferCamera : register(b1)
{
	float4 cameraParams0;
	float4 cameraParams1;
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

float3 CalcBSDF(float NoV, float NoL, float NoH, float VoH, float roughness, float metallic, float3 albedo, float3 F0)
{
	// 微表面 BRDF
	float NDF = DistributionGGX(NoH, roughness);
	float G = GeometrySmithDirect(NoV, NoL, roughness);
	float3 F = FresnelSchlick(saturate(VoH), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * saturate(NoV) * saturate(NoL);
	float3 specular = numerator / max(denominator, 0.001);

	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
	return kD * diffuse + specular;
}

float3 GetIndirectIrradiance(float3 v)
{
	float3 irradiance;
	irradiance.x =
		g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].x +
		g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].x * v.x +
		g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].x * v.y +
		g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].x * v.z +
		g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].x * v.x * v.z +
		g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].x * v.x * v.y +
		g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].x * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].x * v.y * v.z +
		g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].x * (v.z * v.z - v.x * v.x);

	irradiance.y =
		g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].y +
		g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].y * v.x +
		g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].y * v.y +
		g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].y * v.z +
		g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].y * v.x * v.z +
		g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].y * v.x * v.y +
		g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].y * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].y * v.y * v.z +
		g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].y * (v.z * v.z - v.x * v.x);

	irradiance.z =
		g_SHFactor[0] * cbIrradianceSH[0].irradSH[0].z +
		g_SHFactor[1] * cbIrradianceSH[0].irradSH[1].z * v.x +
		g_SHFactor[2] * cbIrradianceSH[0].irradSH[2].z * v.y +
		g_SHFactor[3] * cbIrradianceSH[0].irradSH[3].z * v.z +
		g_SHFactor[4] * cbIrradianceSH[0].irradSH[4].z * v.x * v.z +
		g_SHFactor[5] * cbIrradianceSH[0].irradSH[5].z * v.x * v.y +
		g_SHFactor[6] * cbIrradianceSH[0].irradSH[6].z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * cbIrradianceSH[0].irradSH[7].z * v.y * v.z +
		g_SHFactor[8] * cbIrradianceSH[0].irradSH[8].z * (v.z * v.z - v.x * v.x);

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

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedo = txRT2.Sample(ssLinearWrap, uv).xyz;
	albedo = pow(albedo, 2.2f);

	float roughnessMap = txRT3.Sample(ssLinearWrap, uv).x;
	float roughness = roughnessMap;
	roughness = roughness * roughness;

	float metallicMap = txRT3.Sample(ssLinearWrap, uv).y;
	float metallic = metallicMap;

	float AOMap = txRT3.Sample(ssLinearWrap, uv).z;
	float SSAOMap = txSSAO.Sample(ssLinearWrap, input.tex).x;
	float ssao = 1.0f - SSAOMap;
	float ao = AOMap * ssao;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

	float3 Lo = 0.0f;
	int i;
	for (i = 0; i < NUM_DISTANT_LIGHT; i++)
	{
		float3 LightDirWS = normalize(m_dirLight[i].direction);
		float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_worldViewInverseTranspose));

		float3 L = -LightDirVS;
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float3 LightIlluminance = m_dirLight[i].color * m_dirLight[i].illuminance; // 方向光的Illuminace
		float3 IncidentIlluminance = LightIlluminance * NoL;

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, roughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance; // Output radiance.
	}

    for (i = 0; i < NUM_POINT_LIGHT; i++)
    {
		float3 LightPosVS = mul(float4(m_pointLight[i].position, 1.0f), m_worldView).xyz;
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

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, roughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance * FalloffFactor; // Output radiance.
    }

	for (i = 0; i < NUM_SPOT_LIGHT; i++)
	{
		float3 LightPosVS = mul(float4(m_spotLight[i].position, 1.0f), m_worldView).xyz;
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
		float3 SpotDirVS = normalize(mul(SpotDirWS, (float3x3)m_worldViewInverseTranspose));
		float FalloffFactor = (dot(-SpotDirVS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
		FalloffFactor = saturate(FalloffFactor);
		FalloffFactor = FalloffFactor * FalloffFactor;

		float Factor = d2 / (m_spotLight[i].influenceRadius * m_spotLight[i].influenceRadius);
		FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

		float3 bsdf = CalcBSDF(NoV, NoL, NoH, VoH, roughness, metallic, albedo, F0);
		Lo += bsdf * IncidentIlluminance * FalloffFactor; // Output radiance.
	}

	float3 kS = fresnelSchlickRoughness(NoV, F0, roughness);
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	// test: SH Irrad.
	//float3 IndirectIrradiance1 = txIrradianceMap.Sample(ssLinearWrap, N).xyz;
	//float3 IndirectIrradiance2 = GetIndirectIrradiance(N);
	//float3 IndirectIrradiance = lerp(IndirectIrradiance1, IndirectIrradiance2, m_cubeMapIntensity * 0.1f);

	float3 IndirectIrradiance = txIrradianceMap.Sample(ssLinearWrap, N).xyz;
	
	float3 diffuseIBL = kD * albedo * IndirectIrradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, roughness * 4.0f).rgb; // 4.0 = prefilter mip count - 1.
	float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
	float3 SpecularIBL = preFilteredColor * lerp(envBRDF.xxx, envBRDF.yyy, F0);

	float3 energyCompensation = 1.0f + F0 * (1.0f / envBRDF.yyy - 1.0f);
	SpecularIBL *= energyCompensation;

	float3 Libl = (diffuseIBL + SpecularIBL) * m_cubeMapIntensity * ao;
	float3 color = Libl + Lo;

	//// fast tone-mapping.
	//color = color / (color + 1.0);

	//// gamma.
	//color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
