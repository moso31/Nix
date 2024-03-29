#include "BRDF.fx"
#include "MathSample.fx"

float2 IntegrateBRDF(float2 uv)
{
	float roughness = 1.0f - uv.x;
	float NoV = uv.y;

	float3 V;
	V.x = sqrt(1.0f - NoV * NoV); // sin
	V.y = 0;
	V.z = NoV; // cos
	float3 N = float3(0.0f, 0.0f, 1.0f);

	float A = 0.0f;
	float B = 0.0f;
	const uint NumSamples = 1024;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = reflect(-V, H);
		float NoL = saturate(L.z);
		float NoH = saturate(H.z);
		float VoH = saturate(dot(V, H));
		if (NoL > 0)
		{
			float G = G_GGX_SmithJoint(NoV, NoL, roughness);
			float G_Vis = 4.0f * G * VoH * NoL / NoH;
			float Fc = pow(1 - VoH, 5.0f);
			A += Fc * G_Vis;
			B += G_Vis;
		}
	}
	return float2(A, B) / (float)NumSamples;
}

struct VS_INPUT 
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return float4(IntegrateBRDF(input.tex), 0.0f, 1.0f);
}
