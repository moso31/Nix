#ifndef _MATH_
#define _MATH_

#include "Common.fx"

// ������任���� Vector �� sourceBasis ת���� targetBasis
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
	float3 normalTS = normalMapValue * 2.0f - 1.0f; // �� [0, 1] ת���� [-1, 1] ����
	return ChangeBasisVector(normalTS, normalWS, tangentWS);
}

float3 TangentSpaceToViewSpace(float3 normalMapValue, float3 normalVS, float3 tangentVS)
{
	float3 normalTS = normalMapValue * 2.0f - 1.0f; // �� [0, 1] ת���� [-1, 1] ����
	return ChangeBasisVector(normalTS, normalVS, tangentVS);
}

// �ڱ���ĳ������
// ��ȡ NormalVS���������Ӧ�� TangentVS �� BitangentVS
void GetNTBMatrixVS(float3 NormalVS, out float3 TangentVS, out float3 BitangentVS)
{
	float3 upVector = float3(0, 1, 0);
	if (abs(dot(NormalVS, upVector)) > 0.99) 
		upVector = float3(1, 0, 0); 

	TangentVS = normalize(cross(upVector, NormalVS));
	BitangentVS = cross(NormalVS, TangentVS);
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