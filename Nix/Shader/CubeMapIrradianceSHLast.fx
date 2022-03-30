#include "SphereHarmonic.fx"

#define NUM_THREADS 8
#define NUM_THREAD_COUNT 64

struct ConstantBufferIrradSH
{
	float4 irradSH[9];
};

cbuffer cbImageSizeData : register(b0)
{
	float4 currImgSize;
	float4 nextImgSize;
}

const static float T[5] = { 0.886226925452757f, 1.023326707946489f, 0.495415912200751f, 0.0f, -0.110778365681594f };

StructuredBuffer<ConstantBufferIrradSH> cbSHIrradIn : register(t0);
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
	int inputPixelIndex = DTid.y * currImgSize.x + DTid.x;
	bool isOffScreen = inputPixelIndex > currImgSize.x * currImgSize.y;
	if (isOffScreen)
	{
		for (int i = 0; i < 9; i++) g_shCache[GroupIndex].irradSH[i] = 0.0f;
	}
	else
	{
		for (int i = 0; i < 9; i++) g_shCache[GroupIndex].irradSH[i] = cbSHIrradIn[inputPixelIndex].irradSH[i];
	}

	GroupMemoryBarrierWithGroupSync();
	ParallelSumOfGroupData(GroupIndex);

	int outputPixelIndex = GroupId.y * nextImgSize.x + GroupId.x;

	for (int l = 0; l < 3; l++)
	{
		for (int m = -l; m <= l; m++)
		{
			int index = l * l + l + m;
			//float4 Llm = isOffScreen ? 0.0f : g_shCache[0].irradSH[index];
			//outIrradanceSHCoeffcient[outputPixelIndex].irradSH[index] = Llm * sqrt(NX_4PI / (2.0f * l + 1.0f)) * T[l] * NX_1DIVPI;

			outIrradanceSHCoeffcient[outputPixelIndex].irradSH[index] = g_shCache[0].irradSH[index] * sqrt(NX_4PI / (2.0f * l + 1.0f)) * T[l] * NX_1DIVPI;
		}
	}
}