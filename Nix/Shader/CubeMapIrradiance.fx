#include "PBR.fx"

TextureCube txCubeMap : register(t0);

SamplerState samTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_worldViewInverseTranspose;
	matrix m_projection;
}

float3 GetIrradiance(float3 wi)
{
	// 形成wi, wt(tangent), wb(bitangent) 坐标系
	float3 wb = float3(0.0f, 1.0f, 0.0f);
	float3 wt = normalize(cross(wb, wi));
	wb = cross(wi, wt);
	float3 irradiance = 0.0f;

	float rTheta = NX_PIDIV2;
	float rPhi = NX_2PI;
	float nTheta = rTheta * 0.025f;
	float nPhi = rTheta * 0.025f;
	int nrSamples = 0;
	for (float phi = 0.0f; phi < rPhi; phi += nTheta)
	{
		for (float theta = 0.0f; theta < rTheta; theta += nTheta)
		{
			// spherical to cartesian (in tangent space)
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			float3 sampleVec = tangentSample.x * wt + tangentSample.y * wb + tangentSample.z * wi;

			irradiance += txCubeMap.Sample(samTriLinearSam, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	irradiance = NX_PI * irradiance * (1.0f / float(nrSamples));
	return irradiance;
}

struct VS_INPUT
{
	float3 pos : POSITION;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float3 posL : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posL = float4(input.pos, 1.0f);
	output.posH = mul(float4(input.pos, 1.0f), m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 irradiance = GetIrradiance(normalize(input.posL));
	return float4(irradiance, 1.0f);
}
