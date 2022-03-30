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
	bool isOffScreen = DTid.x >= currImgSize.x || DTid.y >= currImgSize.y;
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
	for (int i = 0; i < 9; i++)
	{
		//float4 accumulatedSHBasis = isOffScreen ? 0.0f : g_shCache[0].irradSH[i];
		outIrradanceSHCoeffcient[outputPixelIndex].irradSH[i] = g_shCache[0].irradSH[i];
	}
}