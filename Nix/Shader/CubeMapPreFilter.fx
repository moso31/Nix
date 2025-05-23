#include "BRDF.fx"
#include "MathSample.fx"

cbuffer ConstantBufferCubeMapObject : register(b0)
{
	matrix m_world;
	matrix m_view;
	matrix m_projection;
}

TextureCube txCubeMap : register(t0);

SamplerState ssLinearWrap : register(s0);

cbuffer ConstantBufferPreFilter : register(b1)
{
	float4 m_paramSize; // x : cubemap size; y : cubemap size inv; z : prefilter map size; w : prefilter map size inv
	float m_perceptualRoughness;
}

// Educational: 更贴近基本理论的简单模型。
// 纯采样，不扩展采样半向量H的立体角，不进行抗噪优化
float3 GetPrefilterEducational(float perceptualRoughness, float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 result = 0;
	const uint NumSamples = 1024;
	float TotalWeight = 0.0f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, perceptualRoughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(dot(N, L));
		if (NoL > 0.0f)
		{
			result += txCubeMap.SampleLevel(ssLinearWrap, L, 0).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return result / TotalWeight;
}

float3 GetPrefilter(float perceptualRoughness, float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 result = 0;
	float roughness = perceptualRoughness;
	const uint NumSamples = 1024;
	float TotalWeight = 0.0;
	const float invLog4 = 1.6609640474436811739351597147447f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(dot(N, L));
		if (NoL > 0.0f)
		{
			float NoH = saturate(dot(N, H));
			float VoH = saturate(dot(V, H));
			float3 D = D_GGX(NoH, roughness);
			float3 pdf = max(D * NoH / (4.0 * VoH), 0.0001);

			float imgSize = 128.0f;
			float saFactor = 4.0 * 6.0 * imgSize * imgSize / (NX_4PI * (float)NumSamples * pdf);
			float TargetMipLevel = roughness == 0.0f ? 0.0f : max(log(saFactor) * invLog4, 0.0);

			result += txCubeMap.SampleLevel(ssLinearWrap, L, TargetMipLevel).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return result / TotalWeight;
}

struct VS_INPUT
{
	float3 pos : POSITION;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float3 posOS : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posOS = float4(input.pos, 1.0f);
	output.posSS = mul(float4(input.pos, 1.0f), m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posSS = mul(output.posSS, m_projection);

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 irradiance = GetPrefilter(m_perceptualRoughness, normalize(input.posOS));
	return float4(irradiance, 1.0f);
}
