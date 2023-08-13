#ifndef _MATH_
#define _MATH_

#include "Common.fx"

// 坐标基变换。将 Vector 从 sourceBasis 转换到 targetBasis
float3 ChangeBasisVector(float3 sourceBasisVector, float3 targetBasisNormal, float3 targetBasisTangent)
{
	float3 N = targetBasisNormal;
	float3 T = normalize(targetBasisTangent - dot(targetBasisTangent, N) * N);
	float3 B = cross(N, T);
	float3x3 mxTBN = float3x3(T, B, N);
	float3 targetBasisVector = mul(sourceBasisVector, mxTBN);
	return targetBasisVector;
}

float3 TangentSpaceToWorldSpace(float3 normalMapValue, float3 normalWS, float3 tangentWS)
{
	float3 normalTS = normalMapValue * 2.0f - 1.0f; // 从 [0, 1] 转换到 [-1, 1] 区间
	return ChangeBasisVector(normalTS, normalWS, tangentWS);
}

float3 TangentSpaceToViewSpace(float3 normalMapValue, float3 normalVS, float3 tangentVS)
{
	float3 normalTS = normalMapValue * 2.0f - 1.0f; // 从 [0, 1] 转换到 [-1, 1] 区间
	return ChangeBasisVector(normalTS, normalVS, tangentVS);
}

float DepthZ01ToLinear(float z01)
{
	float A = cameraParams1.z;
	float B = cameraParams1.w;
	return B / (z01 - A);
}

float3 GetViewDirVS_unNormalized(float2 uv)
{
	uv = uv * 2.0f - 1.0f;
	uv *= float2(1.0f, -1.0f);
	return float3(uv * cameraParams2.zw, 1.0f);
}

#endif // _MATH_