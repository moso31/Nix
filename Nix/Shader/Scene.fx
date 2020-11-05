#include "PBR.fx"

TextureCube txCubeMap : register(t0);
Texture2D txDiffuse : register(t1);
Texture2D txShadowMap : register(t2);
TextureCube txIrradianceMap : register(t3);
TextureCube txPreFilterMap : register(t4);
Texture2D txBRDF2DLUT : register(t5);
SamplerState samLinear : register(s0);
SamplerComparisonState samShadowMap : register(s1);

cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_projection;
}

cbuffer ConstantBufferCamera : register(b1)
{
	float3 m_eyePos;
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

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 posW : POSITION;
	float3 normW : NORMAL;
	float2 tex : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posW = output.posH;
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 0.0), m_worldInverseTranspose).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 pos = input.posW.xyz;
	float3 N = normalize(input.normW);
	float3 V = normalize(m_eyePos - pos);

	// reflection test
	//return txCubeMap.Sample(samLinear, reflect(-V, N)); 
	
	float3 albedoMap = txDiffuse.Sample(samLinear, input.tex).xyz;
	float3 albedo = m_material.albedo;// *albedoMap;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, m_material.metallic);

    // reflectance equation
    float3 Lo = 0.0;
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
		float3 lightPos = m_pointLight[i].position;
		float3 lightColor = m_pointLight[i].color;

        // 第i个光源的入射radiance
        float3 L = normalize(lightPos - pos);
        float3 H = normalize(V + L);
        float distance = length(lightPos - pos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = lightColor * attenuation;

        // 微表面 BRDF
		float NDF = DistributionGGX(N, H, m_material.roughness);
		float G = GeometrySmith(N, V, L, m_material.roughness);
		float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);

		float3 kS = F;
		float3 kD = 1.0 - kS;
		kD *= 1.0 - m_material.metallic;
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

	float3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
	float3 kD = 1.0 - kS;
	kD *= 1.0 - m_material.metallic;
	float3 irradiance = txIrradianceMap.Sample(samLinear, N).xyz;
	float3 diffuseIBL = kD * albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(samLinear, reflect(-V, N), m_material.roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(samLinear, float2(saturate(dot(N, V)), m_material.roughness)).rg;
	float3 SpecularIBL = preFilteredColor * float3(kS * envBRDF.x + envBRDF.y);

	float3 ambient = diffuseIBL + SpecularIBL; // * ao;
	float3 color = ambient + Lo;

	// gamma.
	color = color / (color + 1.0);
	color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
