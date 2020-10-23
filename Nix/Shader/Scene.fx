#include "PBR.fx"

Texture2D txDiffuse : register(t0);
Texture2D txShadowMap : register(t1);
SamplerState samLinear : register(s0);
SamplerComparisonState samShadowMap : register(s1);

cbuffer ConstantBufferPrimitive : register(b0)
{
	matrix m_world;
}

cbuffer ConstantBufferCamera : register(b1)
{
	matrix m_view;
	matrix m_projection;
	float3 m_eyePos;
	float _align16;
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
	output.normW = mul(float4(input.norm, 0.0), m_world).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 N = normalize(input.normW);
	float3 V = normalize(m_eyePos - input.posW);
	
	float3 F0 = 0.04;
	F0 = lerp(F0, m_material.albedo, m_material.metallic);

    // reflectance equation
    float3 Lo = 0.0;
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
		float3 lightPos = m_pointLight[i].position;
		float3 lightColor = m_pointLight[i].color;

        // calculate per-light radiance
        float3 L = normalize(lightPos - input.posW);
        float3 H = normalize(V + L);
        float distance = length(lightPos - input.posW);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = lightColor * attenuation;

        // cook-torrance brdf
		float NDF = DistributionGGX(N, H, m_material.roughness);
		float G = GeometrySmith(N, V, L, m_material.roughness);
		float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 kS = F;
        float3 kD = 1.0 - kS;
        kD *= 1.0 - m_material.metallic;

        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * m_material.albedo / PI + specular) * radiance * NdotL;
    }

	return float4(Lo, 1.0f);
}
