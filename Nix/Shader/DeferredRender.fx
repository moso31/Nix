#include "Common.fx"
#include "Math.fx"
#include "SHIrradianceCommon.fx"
#include "DeferredShadingCommon.fx"

Texture2D txRT0 : register(t0);
Texture2D txRT1 : register(t1);
Texture2D txRT2 : register(t2);
Texture2D txRT3 : register(t3);
Texture2D txRTDepth : register(t4);
TextureCube txCubeMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);
Texture2D txShadowTest : register(t8);

SamplerState ssLinearWrap : register(s0);
SamplerState ssLinearClamp : register(s1);

cbuffer ConstantBufferLight : register(b2)
{
	DistantLight m_dirLight[NUM_DISTANT_LIGHT];
	PointLight m_pointLight[NUM_POINT_LIGHT];
	SpotLight m_spotLight[NUM_SPOT_LIGHT];
}

cbuffer ConstantBufferCubeMap : register(b3)
{
	float4 m_irradSH0123x;
	float4 m_irradSH4567x;
	float4 m_irradSH0123y;
	float4 m_irradSH4567y;
	float4 m_irradSH0123z;
	float4 m_irradSH4567z;
	float3 m_irradSH8xyz;
	float  m_cubeMapIntensity;
	float4 m_cubeMapIrradMode;
}

cbuffer CBufferParams : register(b4)
{
	CBufferDiffuseProfile sssProfData[16];
}

float3 GetIndirectIrradiance(float3 v)
{
	float3 irradiance = GetIrradiance(v, m_irradSH0123x, m_irradSH4567x, m_irradSH0123y, m_irradSH4567y, m_irradSH0123z, m_irradSH4567z, m_irradSH8xyz);
	return irradiance;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float2 tex : TEXCOORD;
};

struct DeferredRenderingResult
{
	float4 Lighting				: SV_Target0;
	float4 LightingEx			: SV_Target1;
	float4 LightingSSSTransmit	: SV_Target2;
	float4 LightingCopy			: SV_Target3;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;
	return output;
}

void PS(PS_INPUT input, out DeferredRenderingResult output)
{
	float2 uv = input.tex;

	uint shadingModel = (uint)(txRT3.Sample(ssLinearWrap, uv).a * 255.0f);

	// get depthZ
	float depth = txRTDepth.Sample(ssLinearClamp, uv).x;
	// convert depth to linear
	float linearDepthZ = DepthZ01ToLinear(depth);
	float3 ViewDirRawVS = GetViewDirVS_unNormalized(uv);
	float3 PositionVS = ViewDirRawVS * linearDepthZ;

	//float a = txRT3.Sample(ssLinearWrap, uv).z;
	//return float4(a.xxx, 1.0f);
	float4 rt1 = txRT1.Sample(ssLinearWrap, uv);
	float3 N = rt1.xyz;
	uint sssProfIndex = asuint(rt1.w);
	float3 V = normalize(-PositionVS);
	float NoV = max(dot(N, V), 0.0);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedo = txRT2.Sample(ssLinearWrap, uv).xyz;

	float roughnessMap = txRT3.Sample(ssLinearWrap, uv).x;
	float roughness = roughnessMap;
	float perceptualRoughness = roughness * roughness;

	float metallicMap = txRT3.Sample(ssLinearWrap, uv).y;
	float metallic = metallicMap;

	float AOMap = txRT3.Sample(ssLinearWrap, uv).z;
	float ao = AOMap;

	float3 ShadowTest = txShadowTest.Sample(ssLinearClamp, uv).xyz;

	if (shadingModel == 0)
	{
		float3 F0 = 0.04;
		F0 = lerp(F0, albedo, metallic);
		albedo *= 1.0f - metallic;

		float3 Lo_diff = 0.0f;
		float3 Lo_spec = 0.0f;
		float3 Ld, Ls;
		int i;
		for (i = 0; i < NUM_DISTANT_LIGHT; i++)
		{
			EvalRadiance_DirLight(m_dirLight[i], V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		for (i = 0; i < NUM_POINT_LIGHT; i++)
		{
			EvalRadiance_PointLight(m_pointLight[i], PositionVS, V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		for (i = 0; i < NUM_SPOT_LIGHT; i++)
		{
			EvalRadiance_SpotLight(m_spotLight[i], PositionVS, V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		float3 NormalWS = mul(N, (float3x3)m_viewTranspose);
		//float3 IndirectIrradiance = txIrradianceMap.Sample(ssLinearWrap, NormalWS).xyz;
		float3 IndirectIrradiance = GetIndirectIrradiance(NormalWS);
		float3 diffuseIBL = albedo * IndirectIrradiance;

		float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, perceptualRoughness * 4.0f).rgb; // 4.0 = prefilter mip count - 1.
		float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
		float3 SpecularIBL = preFilteredColor * lerp(envBRDF.xxx, envBRDF.yyy, F0);

		float3 energyCompensation = 1.0f + F0 * (1.0f / envBRDF.yyy - 1.0f);
		SpecularIBL *= energyCompensation;

		float3 Lo = Lo_diff + Lo_spec;

		Lo *= (1.0f - ShadowTest);
		Lo += (diffuseIBL + SpecularIBL) * m_cubeMapIntensity;
		Lo *= ao;
		output.Lighting = float4(Lo, 1.0f);
		output.LightingEx = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (shadingModel == 1)
	{
		output.Lighting = float4(albedo, 1.0f);
		output.LightingEx = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (shadingModel == 2)
	{
		float3 F0 = 0.04;
		F0 = lerp(F0, albedo, metallic);
		albedo *= 1.0f - metallic; 

		float3 Lo_diff = 0.0f;
		float3 Lo_spec = 0.0f;
		float3 Ld, Ls, Itt;
		int i;
		for (i = 0; i < NUM_DISTANT_LIGHT; i++)
		{
			EvalRadiance_DirLight_SubSurface(sssProfData[sssProfIndex], m_dirLight[i], V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		for (i = 0; i < NUM_POINT_LIGHT; i++)
		{
			EvalRadiance_PointLight_SubSurface(sssProfData[sssProfIndex], m_pointLight[i], PositionVS, V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		for (i = 0; i < NUM_SPOT_LIGHT; i++)
		{
			EvalRadiance_SpotLight_SubSurface(sssProfData[sssProfIndex], m_spotLight[i], PositionVS, V, N, NoV, perceptualRoughness, metallic, albedo, F0, Ld, Ls);
			Lo_diff += Ld;
			Lo_spec += Ls;
		}

		float3 NormalWS = mul(N, (float3x3)m_viewTranspose);
		//float3 IndirectIrradiance = txIrradianceMap.Sample(ssLinearWrap, NormalWS).xyz;
		float3 IndirectIrradiance = GetIndirectIrradiance(NormalWS);
		float3 diffuseIBL = albedo * IndirectIrradiance;

		float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, perceptualRoughness * 4.0f).rgb; // 4.0 = prefilter mip count - 1.
		float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
		float3 SpecularIBL = preFilteredColor * lerp(envBRDF.xxx, envBRDF.yyy, F0);

		float3 energyCompensation = 1.0f + F0 * (1.0f / envBRDF.yyy - 1.0f);
		SpecularIBL *= energyCompensation;

		Lo_diff *= (1.0f - ShadowTest);
		Lo_spec *= (1.0f - ShadowTest);

		Lo_diff += diffuseIBL * m_cubeMapIntensity;
		Lo_spec += SpecularIBL * m_cubeMapIntensity;

		Lo_diff *= ao;
		Lo_spec *= ao;

		output.Lighting = float4(Lo_diff, 1.0f);
		output.LightingEx = float4(Lo_spec, 1.0f);
	}
	else
	{
		output.Lighting = float4(0.0f, 0.0f, 0.0f, 1.0f);
		output.LightingEx = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	output.LightingCopy = output.Lighting;
	output.LightingSSSTransmit = float4(0.0f, 0.0f, 0.0f, 1.0f);
}
