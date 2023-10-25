#include "Math.fx"

// G(x)
float Burley3S_G(float x) 
{
	return pow(1 + 4 * x * (2 * x + sqrt(1 + 4 * x * x)), 1.0f / 3.0f); 
}

// Generate r = P^-1(x)
// s �� ������أ�һ��������� s Խ�ߣ�����Խ����
float Burley3S_InverseCDF(float x, float s)
{
	return 3.0f / s * log((1 + 1 / Burley3S_G(1 - x) + Burley3S_G(1 - x)) / (4 * (1 - x)));
}

// p(r)
// s �� ������أ�һ��������� s Խ�ߣ�����Խ����
float Burley3S_PDF(float x, float s)
{
	return s / 4 * (exp(-s * x) + exp(-s * x / 3)); 
}

float2 GenerateBurley3SDiskUV(float r)
{
	float theta = Random01() * 2PI;
	return float2(r * cos(theta), r * sin(theta));
}

// ����һ��Ԥ��� Noise ���� ���в����������� �����
float RandomSampleFromNoiseGray(Texture2D NoiseTex, float2 uv, uint seed)
{
	float2 noiseUV = frac(uv + float2(seed * 0.37, seed * 0.73));
	return NoiseTex.Sample(NoiseSampler, noiseUV).r; // Burley SSS ʹ�õ�����Ӧ��ֻ�� R ͨ��
}

// �� View-Space Pos ת���� ScreenUV��
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

	float SampleN = 20.0f; // Burley SSS ��������
	float SumWeight = 0.0f;
	for (float i = 0.0f; i < SampleN; i += 1.0f)
	{
		// ���� x
		float Random = Random01(); 

		// �ص���������� r
		float r = Burley3S_InverseCDF(random, s); 

		// Disk UV ������2D Բ������
		float2 DiskUV = GenerateBurley3SDiskUV(r); 

		// �������е�����߿ռ� ���� Disk UV ƫ�ƣ��õ�������
		float3 SamplePos = PositionVS + TangentVS * DiskUV.x + BitangentVS * DiskUV.y;

		// ��������ת������Ļ�ռ� UV
		float2 SampleUV = ConvertPositionVSToScreenUV(SamplePos); 

		// ��ȡ���������ȣ���ת�����������
		float SampleDepth = txDepthZ.Sample(ssLinearClamp, SampleUV).x;
		float SampleLinearDepth = DepthZ01ToLinear(SampleDepth);

		// �����Ϣ��һ��λ�� Disk ��ƽ���ϣ���Ҫ ʹ�ù��ɶ���У�� r ��ֵ
		float depthOffset = SampleLinearDepth - Depth;
		float AdjustR = sqrt(r * r + depthOffset * depthOffset); // У�� r

		// ���ͳ�Ƶ�ǰ�����ķ����� R(r)
		float3 Weight = R(AdjustR) * NX_2PI * AdjustR / Burley3S_PDF(r, s);

		// �ۻ��������
		float3 SampleColor = txIrradiance.Sample(ssLinearClamp, SampleUV).xyz;
		SampleColor *= Weight; // ���ؿ������
		SSSResult += sampleColor / SampleN; 
		SumWeight += Weight;
	}

	SSSResult /= SumWeight;

	//// ͸��
	//float3 TransmissionColor = GetTransmissionColor(); // �û��Զ���͸����ɫ I_{tt}
	//TransmissionColor *= 0.25f * A * (exp(-s * centerDepth) + exp(-s * centerDepth / 3));

	//SSSresult += TransmissionColor;

	float3 spec = txSpecular.Sample(ssLinearClamp, input.tex).xyz;
	float3 result = irradiance + spec;
	return float4(result, 1.0f);
}
