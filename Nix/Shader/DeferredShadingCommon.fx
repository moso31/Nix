#ifndef _DEFERRED_SHADING_COMMON_H_
#define _DEFERRED_SHADING_COMMON_H_

#include "Common.fx"
#include "BRDF.fx"
#include "PBRLights.fx"

void CalcBSDF(float NoV, float NoL, float NoH, float VoH, float roughness, float metallic, float3 albedo, float3 F0, out float3 diffBSDF, out float3 specBSDF)
{
	diffBSDF = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);

	// 微表面 BRDF
	float D = D_GGX(NoH, roughness);
	float G = G_GGX_SmithJoint(NoV, NoL, roughness);
	float3 F = F_Schlick(saturate(VoH), F0);
	specBSDF = D * G * F;
}

void EvalRadiance_DirLight(DistantLight dirLight, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightDirWS = normalize(-dirLight.direction);
	float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_viewInverseTranspose));

	float3 L = LightDirVS;
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float3 LightIlluminance = dirLight.color * dirLight.illuminance; // 方向光的Illuminace
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance;
	Lo_spec = f_spec * IncidentIlluminance;
}

void EvalRadiance_PointLight(PointLight pointLight, float3 CamPosVS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosVS = mul(float4(pointLight.position, 1.0f), m_view).xyz;
	float3 LightIntensity = pointLight.color * pointLight.intensity;
	float3 LightDirVS = LightPosVS - CamPosVS;

	float3 L = normalize(LightDirVS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirVS, LightDirVS);
	float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float Factor = d2 / (pointLight.influenceRadius * pointLight.influenceRadius);
	float FalloffFactor = max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;
}

void EvalRadiance_SpotLight(SpotLight spotLight, float3 CamPosVS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosVS = mul(float4(spotLight.position, 1.0f), m_view).xyz;
	float3 LightIntensity = spotLight.color * spotLight.intensity;
	float3 LightDirVS = LightPosVS - CamPosVS;

	float3 L = normalize(LightDirVS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirVS, LightDirVS);
	float3 LightIlluminance = LightIntensity / (NX_PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float CosInner = cos(spotLight.innerAngle * NX_DEGTORED);
	float CosOuter = cos(spotLight.outerAngle * NX_DEGTORED);
	float3 SpotDirWS = normalize(spotLight.direction);
	float3 SpotDirVS = normalize(mul(SpotDirWS, (float3x3)m_viewInverseTranspose));
	float FalloffFactor = (dot(-SpotDirVS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
	FalloffFactor = saturate(FalloffFactor);
	FalloffFactor = FalloffFactor * FalloffFactor;

	float Factor = d2 / (spotLight.influenceRadius * spotLight.influenceRadius);
	FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;
}


void EvalRadiance_DirLight_SubSurface(DistantLight dirLight, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec, out float3 I_transmit)
{
	float3 LightDirWS = normalize(-dirLight.direction);
	float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_viewInverseTranspose));

	float3 L = LightDirVS;
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float3 LightIlluminance = dirLight.color * dirLight.illuminance; // 方向光的Illuminace
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance;
	Lo_spec = f_spec * IncidentIlluminance;

	float backNoL = (dot(-N, L) + 1.0) * 0.5f;
	I_transmit = dot(LightIlluminance, LightIlluminance) < 1e-4f ? 0.0f : LightIlluminance * backNoL;
}

void EvalRadiance_PointLight_SubSurface(PointLight pointLight, float3 CamPosVS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec, out float3 I_transmit)
{
	float3 LightPosVS = mul(float4(pointLight.position, 1.0f), m_view).xyz;
	float3 LightIntensity = pointLight.color * pointLight.intensity;
	float3 LightDirVS = LightPosVS - CamPosVS;

	float3 L = normalize(LightDirVS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirVS, LightDirVS);
	float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float Factor = d2 / (pointLight.influenceRadius * pointLight.influenceRadius);
	float FalloffFactor = max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;

	float backNoL = (dot(-N, L) + 1.0) * 0.5f;
	I_transmit = dot(LightIlluminance, LightIlluminance) < 1e-4f ? 0.0f : LightIlluminance * backNoL;
}

void EvalRadiance_SpotLight_SubSurface(SpotLight spotLight, float3 CamPosVS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec, out float3 I_transmit)
{
	float3 LightPosVS = mul(float4(spotLight.position, 1.0f), m_view).xyz;
	float3 LightIntensity = spotLight.color * spotLight.intensity;
	float3 LightDirVS = LightPosVS - CamPosVS;

	float3 L = normalize(LightDirVS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirVS, LightDirVS);
	float3 LightIlluminance = LightIntensity / (NX_PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float CosInner = cos(spotLight.innerAngle * NX_DEGTORED);
	float CosOuter = cos(spotLight.outerAngle * NX_DEGTORED);
	float3 SpotDirWS = normalize(spotLight.direction);
	float3 SpotDirVS = normalize(mul(SpotDirWS, (float3x3)m_viewInverseTranspose));
	float FalloffFactor = (dot(-SpotDirVS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
	FalloffFactor = saturate(FalloffFactor);
	FalloffFactor = FalloffFactor * FalloffFactor;

	float Factor = d2 / (spotLight.influenceRadius * spotLight.influenceRadius);
	FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;

	float backNoL = (dot(-N, L) + 1.0) * 0.5f;
	I_transmit = dot(LightIlluminance, LightIlluminance) < 1e-4f ? 0.0f : LightIlluminance * backNoL;
}

#endif // _DEFERRED_SHADING_COMMON_H_