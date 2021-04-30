#include "Common.fx"
#include "PBR.fx"
#include "Math.fx"

Texture2D txRT0 : register(t0);
Texture2D txRT1 : register(t1);
Texture2D txRT2 : register(t2);
Texture2D txRT3 : register(t3);
TextureCube txCubeMap : register(t4);
TextureCube txIrradianceMap : register(t5);
TextureCube txPreFilterMap : register(t6);
Texture2D txBRDF2DLUT : register(t7);

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

float4 PS(PS_INPUT input) : SV_Target
{
	float2 uv = input.tex;
	float3 pos = txRT0.Sample(SamplerStateTrilinear, uv).xyz;
	float3 N = txRT1.Sample(SamplerStateTrilinear, uv).xyz;
	float3 V = normalize(-pos);
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)m_viewTranspose);

	//return txCubeMap.Sample(SamplerStateTrilinear, R);	// perfect reflection test

	float3 albedo = txRT2.Sample(SamplerStateTrilinear, uv).xyz;
	albedo = pow(albedo, 2.2f);

	float roughnessMap = txRT3.Sample(SamplerStateTrilinear, uv).x;
	//float roughness = m_material.roughness;
	float roughness = roughnessMap;

	float metallicMap = txRT3.Sample(SamplerStateTrilinear, uv).y;
	//float metallic = m_material.metallic;
	float metallic = metallicMap;

	float AOMap = txRT3.Sample(SamplerStateTrilinear, uv).z;
	//float metallic = m_material.metallic;
	float ao = AOMap;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

    // reflectance equation
    float3 Lo = 0.0f;
    for (int i = 0; i < 0; i++)
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
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmithDirect(N, V, L, roughness);
		float3 F = fresnelSchlick(saturate(dot(H, V)), F0);

        float3 numerator = NDF * G * F;
        float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
        float3 specular = numerator / max(denominator, 0.001);

		float3 kS = F;
		float3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / NX_PI + specular) * radiance * NdotL;
    }

	float3 kS = fresnelSchlick(saturate(dot(N, V)), F0);
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	float3 irradiance = txIrradianceMap.Sample(SamplerStateTrilinear, N).xyz;
	float3 diffuseIBL = kD * albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(SamplerStateTrilinear, R, roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(SamplerStateTrilinear, float2(saturate(dot(N, V)), roughness)).rg;
	float3 SpecularIBL = preFilteredColor * float3(kS * envBRDF.x + envBRDF.y);

	float3 ambient = (diffuseIBL + SpecularIBL) * ao;
	float3 color = ambient + Lo;

	// gamma.
	color = color / (color + 1.0);
	color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
