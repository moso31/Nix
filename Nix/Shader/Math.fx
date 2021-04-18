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

float LinearDepth(float z, float n, float f)
{
	// z: ͶӰ���ԭ��depth
	// f: camera far
	// n: camera near
	return f * n / (f + (n - f) * z);
}