#include "Common.fx"
#include "PBRLights.fx"
#include "BRDF.fx"
#include "Math.fx"

TextureCube txCubeMap : register(t0);
Texture2D txAlbedo : register(t1);
Texture2D txNormalMap : register(t2);
Texture2D txMetallicMap : register(t3);
Texture2D txRoughnessMap : register(t4);
Texture2D txAmbientOcclusionMap : register(t5);
//Texture2D txHeightMap : register(t6);
TextureCube txIrradianceMap : register(t7);
TextureCube txPreFilterMap : register(t8);
Texture2D txBRDF2DLUT : register(t9);
//Texture2D txShadowMap : register(t10);
Texture2D txSSAO : register(t11);

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

cbuffer ConstantBufferMaterial : register(b3)
{
	Material m_material;
}

cbuffer ConstantBufferShadowMapTransform : register(b4)
{
	matrix m_shadowMapView;
	matrix m_shadowMapProjection;
	matrix m_shadowMapTex;
}

cbuffer ConstantBufferCubeMap : register(b5)
{
	float m_cubeMapIntensity;
	float3 _0;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangent : TANGENT;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float4 posVS : POSITION;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = mul(input.pos, m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posVS = output.posSS;
	output.posSS = mul(output.posSS, m_projection);
	output.normVS = normalize(mul(input.norm, (float3x3)m_worldViewInverseTranspose));
	output.tex = input.tex;
	output.tangentVS = mul(input.tangent, (float3x3)m_worldViewInverseTranspose).xyz;
	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 normalMap = txNormalMap.Sample(ssLinearWrap, input.tex).xyz;
	float3 normal = m_material.normal * normalMap;

	float3 PositionVS = input.posVS.xyz;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	float3 V = normalize(-PositionVS);
	float NoV = max(dot(N, V), 0.0);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	//return txCubeMap.Sample(ssLinearWrap, R);	// perfect reflection test

	float3 albedoMap = txAlbedo.Sample(ssLinearWrap, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;
	albedo = pow(albedo, 2.2f);
	//return float4(albedoMap, 1.0f);	// albedo only test

	float roughnessMap = txRoughnessMap.Sample(ssLinearWrap, input.tex).x;
	float roughness = m_material.roughness * roughnessMap;
	roughness = roughness * roughness;

	float metallicMap = txMetallicMap.Sample(ssLinearWrap, input.tex).x;
	float metallic = m_material.metallic * metallicMap;

	float AOMap = txAmbientOcclusionMap.Sample(ssLinearWrap, input.tex).x;
	float SSAOMap = txSSAO.Sample(ssLinearWrap, input.tex).x;
	float ssao = 1.0f - SSAOMap;
	float ao = m_material.ao * AOMap * ssao;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

    // reflectance equation
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
