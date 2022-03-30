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

Texture2D txHDRMap : register(t0);
SamplerState ssLinearWrap : register(s0);
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
	float2 uv = ((float2)DTid + 0.5f) * currImgSize.zw;
	int outputPixelIndex = GroupId.y * nextImgSize.x + GroupId.x;

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
				float sh = SHBasis(l, m, uv.y * NX_PI, NX_3PIDIV2 - uv.x * NX_2PI);  // HDRI纹理角度矫正

				// sh = y_l^m(Rs)
				// m_shIrradianceMap[k++] = L_l^m
				float4 Llm = L * sh * solidAnglePdf;
				float index = l * l + l + m;
				g_shCache[GroupIndex].irradSH[index] = Llm;
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();
	ParallelSumOfGroupData(GroupIndex);

	for (int i = 0; i < 9; i++)
	{
		float4 accumulatedSHBasis = isOffScreen ? 0.0f : g_shCache[0].irradSH[i];
		outIrradanceSHCoeffcient[outputPixelIndex].irradSH[i] = accumulatedSHBasis;
	}
}
