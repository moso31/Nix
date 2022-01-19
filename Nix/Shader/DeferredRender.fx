#include "Common.fx"
#include "PBRLights.fx"
#include "BRDF.fx"
#include "Math.fx"

Texture2D txRT0 : register(t0);
Texture2D txRT1 : register(t1);
Texture2D txRT2 : register(t2);
Texture2D txRT3 : register(t3);
TextureCube txCubeMap : register(t4);
TextureCube txIrradianceMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);
Texture2D txSSAO : register(t8);

SamplerState ssLinearWrap : register(s0);
SamplerState ssLinearClamp : register(s1);

cbuffer ConstantBufferCamera : register(b1)
{
	float4 cameraParams0;
	float4 cameraParams1;
	float4 cameraParams2;
}

cbuffer ConstantBufferLight : register(b2)
{
	DistantLight m_dirLight[NUM_DISTANT_LIGHT];
	PointLight m_pointLight[NUM_POINT_LIGHT];
	SpotLight m_spotLight[NUM_SPOT_LIGHT];
}

cbuffer ConstantBufferCubeMap : register(b3)
{
	float m_cubeMapIntensity;
	float3 _0;
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

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;
	return output;
}

//void CalcBSDF()
//{
//	// 微表面 BRDF
//	float NDF = DistributionGGX(NoH, roughness);
//	float G = GeometrySmithDirect(NoV, NoL, roughness);
//	float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
//
//	float3 numerator = NDF * G * F;
//	float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
//	float3 specular = numerator / max(denominator, 0.001);
//
//	float3 kS = F;
//	float3 kD = 1.0 - kS;
//	kD *= 1.0 - metallic;
//
//	float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
//	Lo += (kD * diffuse + specular) * IncidentIlluminance; // Output radiance.
//}

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = input.tex;
	float3 PositionVS = txRT0.Sample(ssLinearWrap, uv).xyz;
	float3 N = txRT1.Sample(ssLinearWrap, uv).xyz;
	float3 V = normalize(-PositionVS);
	float NoV = max(dot(N, V), 0.0);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedo = txRT2.Sample(ssLinearWrap, uv).xyz;
	albedo = pow(albedo, 2.2f);

	float roughnessMap = txRT3.Sample(ssLinearWrap, uv).x;
	float roughness = roughnessMap;
	roughness = roughness * roughness;

	float metallicMap = txRT3.Sample(ssLinearWrap, uv).y;
	float metallic = metallicMap;

	float AOMap = txRT3.Sample(ssLinearWrap, uv).z;
	float SSAOMap = txSSAO.Sample(ssLinearWrap, input.tex).x;
	float ssao = 1.0f - SSAOMap;
	float ao = AOMap * ssao;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

	float3 Lo = 0.0f;
	int i;
	for (i = 0; i < NUM_DISTANT_LIGHT; i++)
	{
		float3 LightDirWS = normalize(m_dirLight[i].direction);
		float3 LightDirVS = normalize(mul(LightDirWS, (float3x3)m_worldViewInverseTranspose));

		float3 L = -LightDirVS;
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float3 LightIlluminance = m_dirLight[i].color * m_dirLight[i].illuminance; // 方向光的Illuminace
		float3 IncidentIlluminance = LightIlluminance * NoL;

		// 微表面 BRDF
		float NDF = DistributionGGX(NoH, roughness);
		float G = GeometrySmithDirect(NoV, NoL, roughness);
		float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
		float3 specular = numerator / max(denominator, 0.001);

		float3 kS = F;
		float3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;

		float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
		Lo += (kD * diffuse + specular) * IncidentIlluminance; // Output radiance.
	}

    for (i = 0; i < NUM_POINT_LIGHT; i++)
    {
		float3 LightPosVS = mul(float4(m_pointLight[i].position, 1.0f), m_worldView).xyz;
		float3 LightIntensity = m_pointLight[i].color * m_pointLight[i].intensity;
		float3 LightDirVS = LightPosVS - PositionVS;

        float3 L = normalize(LightDirVS);
		float3 H = normalize(V + L);
		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		float d2 = dot(LightDirVS, LightDirVS);
		float3 LightIlluminance = LightIntensity / (NX_4PI * d2);
		float3 IncidentIlluminance = LightIlluminance * NoL;

		// 微表面 BRDF
		float NDF = DistributionGGX(NoH, roughness);
		float G = GeometrySmithDirect(NoV, NoL, roughness);
		float3 F = FresnelSchlick(VoH, F0);

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * NoV * NoL;
		float3 specular = numerator / max(denominator, 0.001);

		float3 kS = F;
		float3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;

		float3 diffuse = DiffuseDisney(albedo, roughness, NoV, NoL, VoH);
		Lo += (kD * diffuse + specular) * IncidentIlluminance; // Output radiance.
    }

	float3 kS = FresnelSchlick(NoV, F0);
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	float3 irradiance = txIrradianceMap.Sample(ssLinearWrap, N).xyz;
	
	float3 diffuseIBL = kD * albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(NoV, roughness)).rg;
	float3 SpecularIBL = preFilteredColor * float3(kS * envBRDF.x + envBRDF.y);

	float3 ambient = (diffuseIBL + SpecularIBL) * m_cubeMapIntensity * ao;
	float3 color = ambient + Lo;

	// fast tone-mapping.
	color = color / (color + 1.0);

	// gamma.
	color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
