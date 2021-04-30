#include "Common.fx"
#include "PBR.fx"
#include "MathSample.fx"

TextureCube txCubeMap : register(t0);

cbuffer ConstantBufferPreFilter : register(b1)
{
	float m_roughness;
}

// Educational: 更贴近基本理论的简单模型。
// 纯采样，不扩展采样半向量H的立体角，不进行抗噪优化
float3 GetPrefilterEducational(float roughness, float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 result = 0;
	const uint NumSamples = 1024;
	float TotalWeight = 0.0f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(dot(N, L));
		if (NoL > 0.0f)
		{
			result += txCubeMap.SampleLevel(SamplerStateTrilinear, L, 0).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return result / TotalWeight;
}

float3 GetPrefilter(float roughness, float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 result = 0;
	const uint NumSamples = 1024;
	float TotalWeight = 0.0;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(dot(N, L));
		if (NoL > 0.0f)
		{
			float3 D = DistributionGGX(N, H, roughness);
			float NdotH = saturate(dot(N, H));
			float VdotH = saturate(dot(V, H));
			float3 pdf = max(D * NdotH / (4.0 * VdotH), 0.0001);

			float imgSize = 512.0f;
			//float saPerH = 1.0f / (NumSamples * pdf);
			//float saPerTexel = 4.0 * NX_PI / (6.0 * imgSize * imgSize);
			float saFactor = 6.0 * imgSize * imgSize / (NX_4PI * (float)NumSamples * pdf);
			float TargetMipLevel = roughness == 0.0f ? 0.0f : max(0.5 * log2(saFactor), 0.0);

			result += txCubeMap.SampleLevel(SamplerStateTrilinear, L, TargetMipLevel).rgb * NoL;
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
	float3 irradiance = GetPrefilter(m_roughness, normalize(input.posOS));
	return float4(irradiance, 1.0f);
}
