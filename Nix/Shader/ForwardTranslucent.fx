#include "Common.fx"
#include "PBRLights.fx"
#include "BRDF.fx"
#include "Math.fx"
#include "SHIrradianceCommon.fx"

struct PBRMaterialTranslucent
{
	float3 albedo;
	float opacity;
	float3 normal;
	float metallic;
	float roughness;
	float ao;
	float2 _1;
};

TextureCube txCubeMap : register(t0);
Texture2D txAlbedo : register(t1);
Texture2D txNormalMap : register(t2);
Texture2D txMetallicMap : register(t3);
Texture2D txRoughnessMap : register(t4);
Texture2D txAmbientOcclusionMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);
Texture2D txDepthPeeling : register(t8);

SamplerState ssLinearWrap : register(s0);
SamplerState ssLinearClamp : register(s1);
#if ENABLE_DEPTH_PEELING
SamplerState ssPointClamp : register(s2);
#endif

cbuffer ConstantBufferLight : register(b2)
{
	DistantLight m_dirLight[NUM_DISTANT_LIGHT];
	PointLight m_pointLight[NUM_POINT_LIGHT];
	SpotLight m_spotLight[NUM_SPOT_LIGHT];
}

cbuffer CBufferMaterialStandard : register(b3)
{
	PBRMaterialTranslucent m_material;
}

cbuffer ConstantBufferShadowMapTransform : register(b4)
{
	matrix m_shadowMapWorld;
	matrix m_shadowMapView;
	matrix m_shadowMapProj;
}

cbuffer ConstantBufferCubeMap : register(b5)
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

float3 GetIndirectIrradiance(float3 v)
{
	float3 irradiance = GetIrradiance(v, m_irradSH0123x, m_irradSH4567x, m_irradSH0123y, m_irradSH4567y, m_irradSH0123z, m_irradSH4567z, m_irradSH8xyz);
	return irradiance;
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
	float4 posVS : POSITION;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = mul(input.pos, m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posVS = output.posSS;
	output.posSS = mul(output.posSS, m_projection);
	output.normVS = normalize(mul(input.norm, (float3x3)m_worldViewInverseTranspose));
	output.tex = input.tex;
	output.tangentVS = normalize(mul(input.tangent, (float3x3)m_worldViewInverseTranspose));
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
#if ENABLE_DEPTH_PEELING
	float2 uv = input.posSS.xy / float2(1600.0f, 900.0f);
	float depthPeelingPos = txDepthPeeling.Sample(ssPointClamp, uv).x;

	float4 pos = mul(input.posVS, m_projection);
	float currentPos = pos.z / pos.w;
	clip(currentPos - depthPeelingPos - 1e-5f);
#endif

	float3 normalMap = txNormalMap.Sample(ssLinearWrap, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;

	float3 PositionVS = input.posVS.xyz;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	float3 V = normalize(-PositionVS);
	float NoV = max(dot(N, V), 0.0);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedoMap = txAlbedo.Sample(ssLinearWrap, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;

	float opacityMap = txAlbedo.Sample(ssLinearWrap, input.tex).w;
	float opacity = m_material.opacity * opacityMap;

	float roughnessMap = txRoughnessMap.Sample(ssLinearWrap, input.tex).x;
	float roughness = m_material.roughness * roughnessMap;
	roughness = roughness * roughness;

	float metallicMap = txMetallicMap.Sample(ssLinearWrap, input.tex).x;
	float metallic = m_material.metallic * metallicMap;

	float AOMap = txAmbientOcclusionMap.Sample(ssLinearWrap, input.tex).x;
	float ao = m_material.ao * AOMap;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);
	albedo *= 1.0f - metallic;

    // reflectance equation
    float3 Lo = 0.0f;
	int i;
	for (i = 0; i < NUM_DISTANT_LIGHT; i++)
	{
		float3 LightDirWS = normalize(m_dirLight[i].direction);
		float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_viewInverseTranspose));

		float3 L = -LightDirVS;
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float3 LightIlluminance = m_dirLight[i].color * m_dirLight[i].illuminance; // 方向光的Illuminace
		float3 IncidentIlluminance = LightIlluminance * NoL;

		// 微表面 BRDF
		float D = D_GGX(NoH, roughness);
		float G = G_GGX_SmithJoint(NoV, NoL, roughness);
		float3 F = F_Schlick(VoH, F0);
		float3 specular = D * G * F;

		float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
		Lo += (diffuse + specular) * IncidentIlluminance; // Output radiance.
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

		// 微表面 BRDF
		float D = D_GGX(NoH, roughness);
		float G = G_GGX_SmithJoint(NoV, NoL, roughness);
		float3 F = F_Schlick(VoH, F0);
		float3 specular = D * G * F;

		float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
		Lo += (diffuse + specular) * IncidentIlluminance; // Output radiance.
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
		float SpotLambda = (dot(-SpotDirVS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
		SpotLambda = saturate(SpotLambda);
		SpotLambda = SpotLambda * SpotLambda;

		// 微表面 BRDF
		float D = D_GGX(NoH, roughness);
		float G = G_GGX_SmithJoint(NoV, NoL, roughness);
		float3 F = F_Schlick(VoH, F0);
		float3 specular = D * G * F;

		float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
		Lo += (diffuse + specular) * IncidentIlluminance * SpotLambda; // Output radiance.
	}
	
	float3 NormalWS = mul(N, (float3x3)m_viewTranspose);
	float3 irradiance = GetIndirectIrradiance(NormalWS);
	float3 diffuseIBL = albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
	float3 SpecularIBL = preFilteredColor * lerp(envBRDF.xxx, envBRDF.yyy, F0);

	float3 energyCompensation = 1.0f + F0 * (1.0f / envBRDF.yyy - 1.0f);
	SpecularIBL *= energyCompensation;

	float3 Libl = (diffuseIBL + SpecularIBL) * m_cubeMapIntensity * ao;
	float3 color = Libl + Lo;

	return float4(color, opacity);
}
