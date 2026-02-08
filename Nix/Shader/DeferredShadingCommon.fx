#ifndef _DEFERRED_SHADING_COMMON_H_
#define _DEFERRED_SHADING_COMMON_H_

#include "Common.fx"
#include "BRDF.fx"
#include "PBRLights.fx"

struct CBufferDiffuseProfile
{
	// TODO: CBufferDiffuseProfile 做到头文件类，统一管理，现在有多处地方定义了这个struct
	float3 scatterParam; // x y z 的 s 参数不同
	float maxScatterDistance;
	float3 transmit;
	float transmitStrength;
};

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

	float3 L = LightDirWS;
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

void EvalRadiance_PointLight(PointLight pointLight, float3 PosWS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosWS = pointLight.position;
	float3 LightIntensity = pointLight.color * pointLight.intensity;
	float3 LightDirWS = LightPosWS - PosWS;

	float3 L = normalize(LightDirWS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirWS, LightDirWS);
	float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float Factor = d2 / (pointLight.influenceRadius * pointLight.influenceRadius);
	float FalloffFactor = max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;
}

void EvalRadiance_SpotLight(SpotLight spotLight, float3 PosWS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosWS = spotLight.position;
	float3 LightIntensity = spotLight.color * spotLight.intensity;
	float3 LightDirWS = LightPosWS - PosWS;

	float3 L = normalize(LightDirWS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirWS, LightDirWS);
	float3 LightIlluminance = LightIntensity / (NX_PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float CosInner = cos(spotLight.innerAngle * NX_DEGTORED);
	float CosOuter = cos(spotLight.outerAngle * NX_DEGTORED);
	float3 SpotDirWS = normalize(spotLight.direction);
	float FalloffFactor = (dot(-SpotDirWS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
	FalloffFactor = saturate(FalloffFactor);
	FalloffFactor = FalloffFactor * FalloffFactor;

	float Factor = d2 / (spotLight.influenceRadius * spotLight.influenceRadius);
	FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;
}

void EvalRadiance_DirLight_SubSurface(CBufferDiffuseProfile sssProfile, DistantLight dirLight, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightDirWS = normalize(-dirLight.direction);

	float3 L = LightDirWS;
	float3 H = normalize(V + L);
	float NoL = saturate(dot(N, L));
	float NoH = saturate(dot(N, H));
	float VoH = saturate(dot(V, H));

	float3 LightIlluminance = dirLight.color * dirLight.illuminance; // 方向光的Illuminace
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float backNoL = saturate(dot(-N, L));
	float3 IncidentIlluminanceTransmit = LightIlluminance * backNoL;

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance;
	Lo_spec = f_spec * IncidentIlluminance;

	// 对 3s 材质，连背面的光照也一起考虑。但背面只算漫反射
	float3 scatter = sssProfile.scatterParam;
	float thickness = sssProfile.transmitStrength;
	float3 irradTransmit = f_diff * IncidentIlluminanceTransmit;
	float3 Lo_transmit = sssProfile.transmit * irradTransmit * 0.25f * (exp(-scatter * thickness) + 3 * exp(-scatter * thickness / 3));
	Lo_diff += Lo_transmit;
}

void EvalRadiance_PointLight_SubSurface(CBufferDiffuseProfile sssProfile, PointLight pointLight, float3 PosWS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosWS = pointLight.position;
	float3 LightIntensity = pointLight.color * pointLight.intensity;
	float3 LightDirWS = LightPosWS - PosWS;

	float3 L = normalize(LightDirWS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirWS, LightDirWS);
	float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float backNoL = saturate(dot(-N, L));
	float3 IncidentIlluminanceTransmit = LightIlluminance * backNoL;

	float Factor = d2 / (pointLight.influenceRadius * pointLight.influenceRadius);
	float FalloffFactor = max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;

	// 对 3s 材质，连背面的光照也一起考虑。但背面只算漫反射
	float3 scatter = sssProfile.scatterParam;
	float thickness = sssProfile.transmitStrength;
	float3 irradTransmit = f_diff * IncidentIlluminanceTransmit * FalloffFactor;
	float3 Lo_transmit = sssProfile.transmit * irradTransmit * 0.25f * (exp(-scatter * thickness) + 3 * exp(-scatter * thickness / 3));
	Lo_diff += Lo_transmit;
}

void EvalRadiance_SpotLight_SubSurface(CBufferDiffuseProfile sssProfile, SpotLight spotLight, float3 PosWS, float3 V, float3 N, float NoV, float perceptualRoughness, float metallic, float3 albedo, float F0, out float3 Lo_diff, out float3 Lo_spec)
{
	float3 LightPosWS = spotLight.position;
	float3 LightIntensity = spotLight.color * spotLight.intensity;
	float3 LightDirWS = LightPosWS - PosWS;

	float3 L = normalize(LightDirWS);
	float3 H = normalize(V + L);
	float NoL = max(dot(N, L), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float d2 = dot(LightDirWS, LightDirWS);
	float3 LightIlluminance = LightIntensity / (NX_PI * d2);
	float3 IncidentIlluminance = LightIlluminance * NoL;

	float backNoL = saturate(dot(-N, L));
	float3 IncidentIlluminanceTransmit = LightIlluminance * backNoL;

	float CosInner = cos(spotLight.innerAngle * NX_DEGTORED);
	float CosOuter = cos(spotLight.outerAngle * NX_DEGTORED);
	float3 SpotDirWS = normalize(spotLight.direction);
	float FalloffFactor = (dot(-SpotDirWS, L) - CosOuter) / max(CosInner - CosOuter, 1e-4f);
	FalloffFactor = saturate(FalloffFactor);
	FalloffFactor = FalloffFactor * FalloffFactor;

	float Factor = d2 / (spotLight.influenceRadius * spotLight.influenceRadius);
	FalloffFactor *= max(1.0f - Factor * Factor, 0.0f);

	float3 f_diff, f_spec;
	CalcBSDF(NoV, NoL, NoH, VoH, perceptualRoughness, metallic, albedo, F0, f_diff, f_spec);

	Lo_diff = f_diff * IncidentIlluminance * FalloffFactor;
	Lo_spec = f_spec * IncidentIlluminance * FalloffFactor;

	// 对 3s 材质，连背面的光照也一起考虑。但背面只算漫反射
	float3 scatter = sssProfile.scatterParam;
	float thickness = sssProfile.transmitStrength;
	float3 irradTransmit = f_diff * IncidentIlluminanceTransmit * FalloffFactor;
	float3 Lo_transmit = sssProfile.transmit * irradTransmit * 0.25f * (exp(-scatter * thickness) + 3 * exp(-scatter * thickness / 3));
	Lo_diff += Lo_transmit;
}

#endif // _DEFERRED_SHADING_COMMON_H_