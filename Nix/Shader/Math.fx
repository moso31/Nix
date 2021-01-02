float3 TangentSpaceToWorldSpace(float3 normalMapValue, float3 normalWorldSpace, float3 tangentWorldSpace, float2 uv)
{
	float3 normalTangentSpace = normalMapValue * 2.0f - 1.0f; // �� [0, 1] ת���� [-1, 1] ����
	float3 N = normalWorldSpace;
	float3 T = normalize(tangentWorldSpace - dot(tangentWorldSpace, N) * N);
	float3 B = cross(N, T);
	float3x3 mxTBN = float3x3(T, B, N);
	float3 bumpedNormalWorldSpace = mul(normalTangentSpace, mxTBN);
	return bumpedNormalWorldSpace;
}
