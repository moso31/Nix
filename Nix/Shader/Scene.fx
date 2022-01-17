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

const static int NUM_LIGHTS = 1;
cbuffer ConstantBufferLight : register(b2)
{
	PointLight m_pointLight[NUM_LIGHTS];
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
	float4 posWS : POSITION;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = mul(input.pos, m_world);
	output.posSS = mul(output.posSS, m_view);
	output.posWS = output.posSS;
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

	float3 pos = input.posWS.xyz;
	float3 N = TangentSpaceToViewSpace(normal, input.normVS, input.tangentVS);
	float3 V = normalize(-pos);
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
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
		float3 lightPos = m_pointLight[i].position;
		lightPos = mul(lightPos, (float3x3)m_view);
		float3 lightColor = m_pointLight[i].color;

        // 第i个光源的入射radiance
        float3 L = normalize(lightPos - pos);
        float3 H = normalize(V + L);
        float distance = length(lightPos - pos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = lightColor * attenuation;

        // 微表面 BRDF
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmithDirect(N, V, L, roughness);
		float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 numerator = NDF * G * F;
        float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
        float3 specular = numerator / max(denominator, 0.001);

		float3 kS = F;
		float3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / NX_PI + specular) * radiance * NdotL;
    }

	float3 kS = FresnelSchlick(saturate(dot(N, V)), F0);
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	float3 irradiance = txIrradianceMap.Sample(ssLinearWrap, N).xyz;
	float3 diffuseIBL = kD * albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(ssLinearWrap, R, roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(ssLinearClamp, float2(saturate(dot(N, V)), roughness)).rg;
	float3 SpecularIBL = preFilteredColor * float3(kS * envBRDF.x + envBRDF.y);

	float3 ambient = (diffuseIBL + SpecularIBL) * m_cubeMapIntensity * ao;
	float3 color = ambient + Lo;

	//// gamma.
	//color = color / (color + 1.0);
	//color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
