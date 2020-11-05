#include "PBR.fx"
#include "Random.fx"

SamplerState samTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

float4 VS(float3 pos : POSITION) : SV_POSITION
{
	return float4(pos, 1.0f);
}

float4 PS(float4 Pos : SV_POSITION) : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
