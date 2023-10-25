#include "Math.fx"

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

float2 GenerateBurley3SDiskUV(float r)
{
	float theta = Random01() * 2PI;
	return float2(r * cos(theta), r * sin(theta));
}

// 基于一张预设的 Noise 纹理 进行采样，以生成 随机数
float RandomSampleFromNoiseGray(Texture2D NoiseTex, float2 uv, uint seed)
{
	float2 noiseUV = frac(uv + float2(seed * 0.37, seed * 0.73));
	return NoiseTex.Sample(NoiseSampler, noiseUV).r; // Burley SSS 使用的纹理应该只有 R 通道
}

// 将 View-Space Pos 转换成 ScreenUV。
float2 ConvertPositionVSToScreenUV(float3 PositionVS)
{
	float zClipInv = 1.0f / (PositionVS.z * cameraParams1.z + cameraParams1.w);
	float2 PositionNDCxy = PositionVS.xy * cameraParams2.xy * zClipInv;
	float2 ScreenUV = (PositionNDCxy + 1.0f) * 0.5f;
	return ScreenUV;
}

Texture2D txIrradiance : register(t0);
Texture2D txSpecular : register(t1);
Texture2D txNormal : register(t2);
Texture2D txDepthZ : register(t3);
Texture2D txNoiseGray : register(t4);
SamplerState ssLinearClamp : register(s0);

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
	float s = 0.5f;

	float Depth = txDepthZ.Sample(ssLinearClamp, uv).x;
	float LinearDepthZ = DepthZ01ToLinear(Depth);
	float3 ViewDirRawVS = GetViewDirVS_unNormalized(uv);
	float3 PositionVS = ViewDirRawVS * LinearDepthZ;

	float3 Irradiance = txIrradiance.Sample(ssLinearClamp, input.tex).xyz;
	float3 NormalVS = txNormal.Sample(ssLinearClamp, input.tex).xyz;
	float3 TangentVS, BitangentVS;
	GetNTBMatrixVS(NormalVS, TangentVS, BitangentVS);

	float3 SSSResult = 0.0f;

	float SampleN = 20.0f; // Burley SSS 采样次数
	float SumWeight = 0.0f;
	for (float i = 0.0f; i < SampleN; i += 1.0f)
	{
		// 生成 x
		float Random = Random01(); 

		// 重点采样，生成 r
		float r = Burley3S_InverseCDF(random, s); 

		// Disk UV 采样，2D 圆盘样本
		float2 DiskUV = GenerateBurley3SDiskUV(r); 

		// 基于命中点的切线空间 进行 Disk UV 偏移，得到采样点
		float3 SamplePos = PositionVS + TangentVS * DiskUV.x + BitangentVS * DiskUV.y;

		// 将采样点转换回屏幕空间 UV
		float2 SampleUV = ConvertPositionVSToScreenUV(SamplePos); 

		// 获取采样点的深度，并转换成线性深度
		float SampleDepth = txDepthZ.Sample(ssLinearClamp, SampleUV).x;
		float SampleLinearDepth = DepthZ01ToLinear(SampleDepth);

		// 深度信息不一定位于 Disk 切平面上，需要 使用勾股定理校正 r 的值
		float depthOffset = SampleLinearDepth - Depth;
		float AdjustR = sqrt(r * r + depthOffset * depthOffset); // 校正 r

		// 最后，统计当前样本的反射率 R(r)
		float3 Weight = R(AdjustR) * NX_2PI * AdjustR / Burley3S_PDF(r, s);

		// 累积采样结果
		float3 SampleColor = txIrradiance.Sample(ssLinearClamp, SampleUV).xyz;
		SampleColor *= Weight; // 蒙特卡洛采样
		SSSResult += sampleColor / SampleN; 
		SumWeight += Weight;
	}

	SSSResult /= SumWeight;

	//// 透射
	//float3 TransmissionColor = GetTransmissionColor(); // 用户自定义透射颜色 I_{tt}
	//TransmissionColor *= 0.25f * A * (exp(-s * centerDepth) + exp(-s * centerDepth / 3));

	//SSSresult += TransmissionColor;

	float3 spec = txSpecular.Sample(ssLinearClamp, input.tex).xyz;
	float3 result = irradiance + spec;
	return float4(result, 1.0f);
}
