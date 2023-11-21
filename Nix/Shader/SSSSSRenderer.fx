#include "Math.fx"
#include "PBRLights.fx"

// R(r)
float3 Burley3S_R(float r, float3 s)
{
    return s * (exp(-s * r) + exp(-s * r / 3)) / (8 * NX_PI * r);
}

// G(x)
float Burley3S_G(float x)
{
	return pow(1 + 4 * x * (2 * x + sqrt(1 + 4 * x * x)), 1.0f / 3.0f);
}

// Generate r = P^-1(x)
// s 和 介质相关，一般可以理解成 s 越高，介质越稠密
float Burley3S_InverseCDF(float x, float s)
{
	return 3.0f / s * log((1 + 1 / Burley3S_G(1 - x) + Burley3S_G(1 - x)) / (4 * (1 - x)));
}

// p(r)
// s 和 介质相关，一般可以理解成 s 越高，介质越稠密
float Burley3S_PDF(float x, float s)
{
	return s / 4 * (exp(-s * x) + exp(-s * x / 3));
}

// 将 View-Space Pos 转换成 ScreenUV。
float2 ConvertPositionVSToScreenUV(float3 positionVS)
{
	float wClipInv = 1.0f / positionVS.z;
	float2 positionNDCxy = positionVS.xy * cameraParams2.xy * wClipInv * float2(1.0f, -1.0f);
	float2 screenUV = (positionNDCxy + 1.0f) * 0.5f;
	return screenUV;
}

Texture2D txIrradiance : register(t0);
Texture2D txSpecular : register(t1);
Texture2D txIrradianceTransmit : register(t2);
Texture2D txNormal : register(t3);
Texture2D txDepthZ : register(t4);
Texture2D txNoiseGray : register(t5);
SamplerState ssLinearClamp : register(s0);

struct CBufferDiffuseProfile
{
	// TODO: CBufferDiffuseProfile 做到头文件类，统一管理，现在有多处地方定义了这个struct
	float3 scatterParam; // x y z 的 s 参数不同
	float maxScatterDistance;
	float3 transmit;
	float transmitStrength;
};

cbuffer CBufferParams : register(b3)
{
	CBufferDiffuseProfile sssProfData[16];
}

// 基于一张预设的 Noise 纹理 进行采样，以生成 0~1 区间内的随机数
float Random01FromNoiseGray(float2 coord, float seed)
{
	float2 noiseUV = frac(coord + float2(seed * 1.14, seed * 5.14));
	return txNoiseGray.Sample(ssLinearClamp, noiseUV).r; // Burley SSS 使用的纹理应该只有 R 通道
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

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = input.tex;
	float2 screenCoord = uv * cameraParams0.xy;

	float depth = txDepthZ.Sample(ssLinearClamp, uv).x;
	float centerLinearDepth = DepthZ01ToLinear(depth);
	float3 viewDirRawVS = GetViewDirVS_unNormalized(uv);
	float3 positionVS = viewDirRawVS * centerLinearDepth;

	float3 irradiance = txIrradiance.Sample(ssLinearClamp, uv).xyz;
	float4 rt1 = txNormal.Sample(ssLinearClamp, uv);
	float3 normalVS = rt1.xyz;
	float3 tangentVS, bitangentVS;
	GetNTBMatrixVS(normalVS, tangentVS, bitangentVS);

	uint sssProfIndex = asuint(rt1.w);
	float3 scatter = sssProfData[sssProfIndex].scatterParam;
	float minScatter = min(scatter.x, min(scatter.y, scatter.z)); // 最小值
	float3 transmit = sssProfData[sssProfIndex].transmit;
	float t = sssProfData[sssProfIndex].transmitStrength;

	float3 sssResult = 0.0f;

	float sampleN = 100.0f; // Burley SSS 采样次数
	float sampleInv = rcp(sampleN);
	float sumWeight = 0.0f;
	for (float i = 0.5f; i < sampleN; i += 1.0f)
	{
		// 准备两个0~1范围数
		float e1 = i * sampleInv; // r 均匀生成
		float e2 = Random01FromNoiseGray(screenCoord, i); // 旋转角 theta 随机生成

		// 反演 e1，获取实际的 r
		// 反演时使用 rgb 通道中 最小的 scatter，确保三个通道的采样范围估计都是完整的
		float r = Burley3S_InverseCDF(e1, minScatter);

		// 均匀采样，生成 theta
		float theta = NX_2PI * e2;

		// Disk UV 采样，2D 圆盘样本
		// same as float2 diskUV = float2(r * sin(theta), r * cos(theta));
		float2 diskUV;
		sincos(theta, diskUV.x, diskUV.y);
		diskUV *= r;

		// 基于命中点的切线空间 进行 Disk UV 偏移，得到采样点
		float3 samplePos = positionVS + tangentVS * diskUV.x + bitangentVS * diskUV.y;

		// 将采样点转换回屏幕空间 UV
		float2 sampleUV = ConvertPositionVSToScreenUV(samplePos);

		// 获取采样点的深度，并转换成线性深度
		float sampleDepth = txDepthZ.Sample(ssLinearClamp, sampleUV).x;
		float sampleLinearDepth = DepthZ01ToLinear(sampleDepth);

		// 深度信息不一定位于 Disk 切平面上，需要 确定实际的 r 的值
		float3 sampleVS = GetViewDirVS_unNormalized(sampleUV) * sampleLinearDepth;
		float adjustR = distance(sampleVS, positionVS); // 获取实际的 r

		// 最后，统计当前样本的反射率 R(r)
		// 算 pdf 时，仍然用初始 r 值计算
		float3 weight = Burley3S_R(adjustR, scatter) * NX_2PI * adjustR / Burley3S_PDF(r, minScatter);

		// 累积采样结果
		sssResult += txIrradiance.Sample(ssLinearClamp, sampleUV).xyz * weight;
		sumWeight += weight;
	}

	sssResult /= sumWeight;

	float3 spec = txSpecular.Sample(ssLinearClamp, uv).xyz;
	float3 result = sssResult + spec;
	return float4(result, 1.0f);
}
