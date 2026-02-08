#include "Common.fx"
#include "Math.fx"

#define TEST_ONLY_SIMPLE_SHADOWMAP 0
#define TEST_SHOW_CASCADE_RANGE 0

Texture2D txSceneDepth : register(t0);
Texture2DArray txShadowMapDepth : register(t1);

SamplerState ssPointClamp : register(s0);

cbuffer ConstantBufferShadowMapObject : register(b2)
{
	matrix m_shadowMapView[8];
	matrix m_shadowMapProj[8];

	// 记录 CSM 各级 用于计算Transition的过渡 的信息。
	float4 m_frustumParams[8]; // x: frustum far; y : transition length

	int m_cascadeCount;
	float m_shadowDistance;
	float m_cascadeTransitionScale;
	int m_depthBias;

	float test_transition;
	float3 _0;
}

// Shadow map
static const float SHADOWMAP_SIZE = 2048.0f;
static const float SHADOWMAP_INVSIZE = 1.0f / SHADOWMAP_SIZE;
static const float DEPTH_BIAS_FACTOR = 1.0f / 100000.0f;

// 阴影贴图PCF。
// 【note 2022.5.14】实际上对NxN块最边缘的像素插值就可以了，待改进
// 【note 2022.6.28】for循环的遍历太渣了，待改进
float ShadowMapPCF(Texture2DArray shadowMapTex, SamplerState ss, float2 uv, float pixelDepth, int cascadeIndex)
{
	float depthbias = m_depthBias * DEPTH_BIAS_FACTOR;

#if TEST_ONLY_SIMPLE_SHADOWMAP
	float4 shadowDepth = shadowMapTex.Sample(ss, float3(uv, (float)cascadeIndex));
	float4 result = step(shadowDepth + depthbias, pixelDepth);
	return result;
#else
	int gridSize = 3;
	float result = 0.0f;
	for (int i = -gridSize; i <= gridSize; i++)
	{
		for (int j = -gridSize; j <= gridSize; j++)
		{
			float2 uvOffset = float2(i * 1.0f, j * 1.0f) * 1.0f * SHADOWMAP_INVSIZE;
			float3 sampleUV = float3(uv + uvOffset, 1.0f);

			float2 texelPos = sampleUV.xy * SHADOWMAP_SIZE;
			float2 texelPosFraction = frac(texelPos);

			float2 samplePosCenter = floor(texelPos) + 0.5f;
			samplePosCenter *= SHADOWMAP_INVSIZE;

			float4 currPixelDepthQuad = shadowMapTex.Gather(ss, float3(samplePosCenter, (float)cascadeIndex));

			// 比较深度，判断是否位于阴影中
			float4 depth = step(currPixelDepthQuad + depthbias, pixelDepth);

			// 对周围4个像素的结果做双线性插值，获取更平滑的深度
			float d1 = lerp(depth.w, depth.z, texelPosFraction.x);
			float d2 = lerp(depth.x, depth.y, texelPosFraction.x);
			float d = lerp(d1, d2, texelPosFraction.y);
			result += d;
		}
	}

	return result / (float)((gridSize * 2 + 1) * (gridSize * 2 + 1));
#endif
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;

	return output;
}

float DoShadowTest(float4 worldPosition, int cascadeIndex)
{
	float4 posInShadowMapSS = mul(worldPosition, m_shadowMapView[cascadeIndex]);
	posInShadowMapSS = mul(posInShadowMapSS, m_shadowMapProj[cascadeIndex]);
	posInShadowMapSS /= posInShadowMapSS.w;

	float2 posInShadowMapUV = (posInShadowMapSS.xy + 1.0f) * 0.5f;
	posInShadowMapUV.y = 1.0f - posInShadowMapUV.y;
	float pixelDepth = posInShadowMapSS.z;

	return ShadowMapPCF(txShadowMapDepth, ssPointClamp, posInShadowMapUV, pixelDepth, cascadeIndex);
}

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = input.tex;

	// get depthZ
	float depth = txSceneDepth.Sample(ssPointClamp, uv).x;
	// convert depth to linear
	float linearDepthZ = DepthZ01ToLinear(depth);
	float3 ViewDirRawVS = GetViewDirVS_unNormalized(uv);
	float3 PositionVS = ViewDirRawVS * linearDepthZ;

	// get View-space position from GBuffer
	float4 posVS = float4(PositionVS, 1.0f);
	float4 posWS = mul(posVS, m_viewInverse);
	
	int cascadeIndex = -1;
	for (int i = m_cascadeCount - 1; i >= 0; i--)
	{
		// 2022.5.16 姑且先使用z距离判断。
		// 也可以使用外接球半径判断处于哪个cascade————
		// 但如果那么做，transition要怎么处理？我暂时没想好。
		if (posVS.z < m_frustumParams[i].x) cascadeIndex = i;
	}

	float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (cascadeIndex == -1)
	{
		return result;
	}

#if TEST_SHOW_CASCADE_RANGE
	if (cascadeIndex == 0) result = float4(0.0f, 0.0f, 1.0f, 1.0f);
	if (cascadeIndex == 1) result = float4(0.0f, 1.0f, 0.0f, 1.0f);
	if (cascadeIndex == 2) result = float4(0.8f, 0.0f, 0.8f, 1.0f);
	if (cascadeIndex == 3) result = float4(1.0f, 0.0f, 0.0f, 1.0f);

	return 1.0f - result;
#endif

	result = DoShadowTest(posWS, cascadeIndex);

	// Transition.
	if (cascadeIndex < m_cascadeCount - 1 && test_transition > 0.5f)
	{
		float resultNext = DoShadowTest(posWS, cascadeIndex + 1);

		float transitionNear = m_frustumParams[cascadeIndex].x - m_frustumParams[cascadeIndex].y;
		float transitionFar = m_frustumParams[cascadeIndex].x;
		float transitionFactor = saturate((posVS.z - transitionNear) / (transitionFar - transitionNear));
		result = lerp(result, resultNext, transitionFactor);
	}

	return result;
}
