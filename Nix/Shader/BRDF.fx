#include "BRDFCommon.fx"
#include "PBRLights.fx"
#include "PBRMaterials.fx"

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = NX_PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGXDirect(float NdotV, float roughness)
{
	float a = roughness;
	float a1 = a + 1;
	float k = a1 * a1 * 0.125;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + max(k, 0.00001); // 加epsilon以防止除0

	return nom / denom;
}

float GeometrySchlickGGXIBL(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + max(k, 0.00001); // 加epsilon以防止除0

	return nom / denom;
}

float GeometrySmithDirect(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float ggx2 = GeometrySchlickGGXDirect(NdotV, roughness);
	float ggx1 = GeometrySchlickGGXDirect(NdotL, roughness);
	return ggx1 * ggx2;
}

float GeometrySmithIBL(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float ggx2 = GeometrySchlickGGXIBL(NdotV, roughness);
	float ggx1 = GeometrySchlickGGXIBL(NdotL, roughness);
	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta);
}

// [Burley 2012, "Physically-Based Shading at Disney"] 
float3 DiffuseDisney(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
{
	float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
	float FdV = 1 + (FD90 - 1) * Pow5(1 - NoV);
	float FdL = 1 + (FD90 - 1) * Pow5(1 - NoL);
	return DiffuseColor * ((1 / NX_PI) * FdV * FdL);
}

float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
	float a = roughness * roughness;
	float Phi = NX_2PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);
	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}
