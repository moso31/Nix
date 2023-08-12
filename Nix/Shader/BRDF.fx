#ifndef _BRDF_
#define _BRDF_

#include "BRDFCommon.fx"

float D_GGX(float NoH, float roughness)
{
	float a2 = roughness * roughness;

	float denom = (NoH * NoH * (a2 - 1.0) + 1.0);
	denom = NX_PI * denom * denom;

	return a2 / max(denom, 1e-5f);
}

float G_GGX_SmithJoint_Lambda(float NoV, float roughness)
{
	float a2 = roughness * roughness;
	return sqrt(NoV * NoV * (1.0f - a2) + a2);
}

float G_GGX_SmithJoint(float NoV, float NoL, float roughness)
{
	float denom = NoL * G_GGX_SmithJoint_Lambda(NoV, roughness) + NoV * G_GGX_SmithJoint_Lambda(NoL, roughness);
	return 0.5f / max(denom, 1e-5f);
}

float3 F_Schlick(float cosTheta, float3 F0)
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

float3 DiffuseLambert(float3 DiffuseColor)
{
	return DiffuseColor / NX_PI;
}

float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a2 = Roughness * Roughness;
	float Phi = NX_2PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a2 - 1) * Xi.y));
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

#endif // !_BRDF_