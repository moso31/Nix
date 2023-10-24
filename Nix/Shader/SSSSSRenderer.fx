// G(x)
float Burley3S_G(float x) 
{
	return pow(1 + 4 * x * (2 * x + sqrt(1 + 4 * x * x)), 1.0f / 3.0f); 
}

// Generate r = P^-1(x)
float Burley3S_InverseCDF(float x)
{
	return 3.0f / s * log((1 + 1 / Burley3S_G(1 - x) + Burley3S_G(1 - x)) / (4 * (1 - x)));
}

// p(r)
float Burley3S_PDF(float x)
{
	return s / 4 * (exp(-s * x) + exp(-s * x / 3)); 
}

float GenerateBurley3SSample()
{
	float x = Random01(); // 0~1
	float r = Burley3S_InverseCDF(x);
	return r;
}

float2 GenerateBurley3SDiskUV(float r)
{
	float theta = Random01() * 2PI;
	return float2(r * cos(theta), r * sin(theta));
}

Texture2D txIrradiance : register(t0);
Texture2D txSpecular : register(t1);
Texture2D txNormal : register(t2);
Texture2D txDepthZ : register(t3);
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
	float depth = txDepthZ.Sample(ssLinearClamp, uv).x;
	float linearDepthZ = DepthZ01ToLinear(depth);
	float3 ViewDirRawVS = GetViewDirVS_unNormalized(uv);
	float3 PositionVS = ViewDirRawVS * linearDepthZ;

	float3 irradiance = txIrradiance.Sample(ssLinearClamp, input.tex).xyz;
	float3 NormalVS = txNormal.Sample(ssLinearClamp, input.tex).xyz;
	float3 TangentVS, BitangentVS;
	GetNTBMatrixVS(NormalVS, TangentVS, BitangentVS);

	float3 SSSResult = 0.0f;

	for (int i = -3; i <= 3; i++)
	{
		for (int j = -3; j <= 3; j++)
		{
			float2 uvOffset = float2(i, j) / 100.0f;
				irradiance += txIrradiance.Sample(ssLinearClamp, input.tex + uvOffset).xyz;
		}
	}
	irradiance /= 49.0f;

	float3 spec = txSpecular.Sample(ssLinearClamp, input.tex).xyz;
	float3 result = irradiance + spec;
	return float4(result, 1.0f);
}
