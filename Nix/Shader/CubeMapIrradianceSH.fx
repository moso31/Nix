#include "SphereHarmonic.fx"

#define NUM_THREADS 8
#define NUM_THREAD_COUNT NUM_THREADS * NUM_THREADS

struct ConstantBufferIrradSH
{
	float4 irradSH[9];
};

cbuffer cbImageSizeData : register(b0)
{
	float4 currImgSize;
	float4 nextImgSize;
}

#if CUBEMAP_IRRADSH_LAST
const static float T[5] = { 0.886226925452757f, 1.023326707946489f, 0.495415912200751f, 0.0f, -0.110778365681594f };
#endif

#if CUBEMAP_IRRADSH_FIRST
Texture2D txHDRMap : register(t0);
SamplerState ssLinearWrap : register(s0);
#else
StructuredBuffer<ConstantBufferIrradSH> cbSHIrradIn : register(t0);
#endif

RWStructuredBuffer<ConstantBufferIrradSH> outIrradanceSHCoeffcient : register(u0);
groupshared ConstantBufferIrradSH g_shCache[NUM_THREAD_COUNT];

void ParallelSumOfGroupData(int GroupIndex)
{
	for (int t = NUM_THREAD_COUNT / 2; t > 0; t >>= 1)
	{
		if (GroupIndex < t)
		{
			for (int i = 0; i < 9; i++)
				g_shCache[GroupIndex].irradSH[i] += g_shCache[GroupIndex + t].irradSH[i];
		}
		GroupMemoryBarrierWithGroupSync();
	}
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void CS(int2 DTid : SV_DispatchThreadID, 
	int2 GroupId : SV_GroupID, 
	int GroupIndex : SV_GroupIndex)
{
#if CUBEMAP_IRRADSH_FIRST
	float2 uv = ((float2)DTid + 0.5f) * currImgSize.zw;

	bool isOffScreen = uv.x > 1.0f || uv.y > 1.0f;
	if (isOffScreen)
	{
		for(int i = 0; i < 9; i++) g_shCache[GroupIndex].irradSH[i] = 0.0f;
	}
	else
	{
		float4 L = txHDRMap.SampleLevel(ssLinearWrap, uv, 0);

		float scaleY = 0.5 * currImgSize.w;
		float thetaU = (uv.y - scaleY) * NX_PI;
		float thetaD = (uv.y + scaleY) * NX_PI;

		float dPhi = NX_2PI * currImgSize.z;	// dPhi 是个常量
		float dTheta = cos(thetaU) - cos(thetaD);
		float solidAnglePdf = dPhi * dTheta;

		for (int l = 0; l < 3; l++)
		{
			for (int m = -l; m <= l; m++)
			{
				float sh = SHBasis(l, m, uv.y * NX_PI, (uv.x - 0.25) * NX_2PI);  // HDRI纹理角度矫正

				// sh = y_l^m(Rs)
				// m_shIrradianceMap[k++] = L_l^m
				float4 Llm = L * sh * solidAnglePdf;
				float index = l * l + l + m;
				g_shCache[GroupIndex].irradSH[index] = Llm;
			}
		}
	}
#else
	int inputPixelIndex = DTid.y * currImgSize.x + DTid.x;
	bool isOffScreen = DTid.x >= currImgSize.x || DTid.y >= currImgSize.y;
	if (isOffScreen)
	{
		for (int i = 0; i < 9; i++) g_shCache[GroupIndex].irradSH[i] = 0.0f;
	}
	else
	{
		for (int i = 0; i < 9; i++) g_shCache[GroupIndex].irradSH[i] = cbSHIrradIn[inputPixelIndex].irradSH[i];
	}
#endif

	GroupMemoryBarrierWithGroupSync();
	ParallelSumOfGroupData(GroupIndex);

	int outputPixelIndex = GroupId.y * nextImgSize.x + GroupId.x;

	for (int l = 0; l < 3; l++)
	{
		for (int m = -l; m <= l; m++)
		{
			int index = l * l + l + m;
			float4 accumulatedIrradianceSH = g_shCache[0].irradSH[index];
#if CUBEMAP_IRRADSH_LAST
			accumulatedIrradianceSH *= sqrt(NX_4PI / (2.0f * l + 1.0f)) * T[l] * NX_1DIVPI;
#endif
			outIrradanceSHCoeffcient[outputPixelIndex].irradSH[index] = accumulatedIrradianceSH;
		}
	}
}
