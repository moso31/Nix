#include "BRDFCommon.fx"
#include "PBRMaterials.fx"

float DistributionGGX(float NoH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float num = a2;
	float denom = (NoH * NoH * (a2 - 1.0) + 1.0);
	denom = NX_PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGXDirect(float NoV, float roughness)
{
	float a = roughness;
	float a1 = a + 1;
	float k = a1 * a1 * 0.125;

	float nom = NoV;
	float denom = NoV * (1.0 - k) + max(k, 0.00001); // 加epsilon以防止除0

	return nom / denom;
}

float GeometrySchlickGGXIBL(float NoV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom = NoV;
	float denom = NoV * (1.0 - k) + max(k, 0.00001); // 加epsilon以防止除0

	return nom / denom;
}

float GeometrySmithDirect(float NoV, float NoL, float roughness)
{
	float ggx2 = GeometrySchlickGGXDirect(NoV, roughness);
	float ggx1 = GeometrySchlickGGXDirect(NoL, roughness);
	return ggx1 * ggx2;
}

float GeometrySmithIBL(float NoV, float NoL, float roughness)
{
	float ggx2 = GeometrySchlickGGXIBL(NoV, roughness);
	float ggx1 = GeometrySchlickGGXIBL(NoL, roughness);
	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
}

// [Burley 2012, "Physically-Based Shading at Disney"] 
float3 DiffuseDisney(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
{
	float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
	float FdV = 1 + (FD90 - 1) * Pow5(1 - NoV);
	float FdL = 1 + (FD90 - 1) * Pow5(1 - NoL);
	return DiffuseColor * ((1 / NX_PI) * FdV * FdL);
}

float3 DiffuseLambert(float3 DiffuseColor)
{
	return DiffuseColor / NX_PI;
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
