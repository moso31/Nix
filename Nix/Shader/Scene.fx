#include "PBR.fx"

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
	float3 tangent : TANGENT;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float4 posW : POSITION;
	float3 normW : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentW : TANGENT;
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
	//output.tangentW = input.tangent;
	output.tangentW = mul(input.tangent, (float3x3)m_world).xyz;
	return output;
}

float3 test(float3 V, float3 A, float3 B, float3 C)
{
	float x = V.x * A.x + V.y * B.x + V.z * C.x;
	float y = V.x * A.y + V.y * B.y + V.z * C.y;
	float z = V.x * A.z + V.y * B.z + V.z * C.z;
	return float3(x, y, z);
}

float3 TangentSpaceToWorldSpace(float3 normalMapValue, float3 normalWorldSpace, float3 tangentWorldSpace, float2 uv)
{
	float3 normalTangentSpace = normalMapValue * 2.0f - 1.0f; // 从 [0, 1] 转换到 [-1, 1] 区间
	float3 N = normalWorldSpace;
	float3 T = normalize(tangentWorldSpace - dot(tangentWorldSpace, N) * N); 
	float3 B = cross(N, T);
	float3x3 mxTBN = float3x3(T, B, N);
	float3 bumpedNormalWorldSpace = mul(normalTangentSpace, mxTBN);
	return bumpedNormalWorldSpace;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float3 normalMap = txNormalMap.Sample(samLinear, input.tex).xyz;
	//return float4(input.tangentW, 1.0f);

	float3 pos = input.posW.xyz;
	//float3 N = normalize(input.normW);
	float3 N = TangentSpaceToWorldSpace(normalMap, input.normW, input.tangentW, input.tex);
	float3 V = normalize(m_eyePos - pos);

	//return float4(N, 1.0f);
	//return txCubeMap.Sample(samLinear, reflect(-V, N));	// perfect reflection test

	float3 albedoMap = txAlbedo.Sample(samLinear, input.tex).xyz;
	float3 albedo = m_material.albedo * albedoMap;
	albedo = pow(albedo, 2.2f);
	//return float4(albedoMap, 1.0f);

	float roughnessMap = txRoughnessMap.Sample(samLinear, input.tex).x;
	//float roughness = m_material.roughness;
	float roughness = roughnessMap;

	float metallicMap = txMetallicMap.Sample(samLinear, input.tex).x;
	//float metallic = m_material.metallic;
	float metallic = metallicMap;

	float AOMap = txAmbientOcclusionMap.Sample(samLinear, input.tex).x;
	//float metallic = m_material.metallic;
	float ao = AOMap;

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

    // reflectance equation
    float3 Lo = 0.0f;
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
	float3 irradiance = txIrradianceMap.Sample(samLinear, N).xyz;
	float3 diffuseIBL = kD * albedo * irradiance;

	float3 preFilteredColor = txPreFilterMap.SampleLevel(samLinear, reflect(-V, N), roughness * 4.0f).rgb;
	float2 envBRDF = txBRDF2DLUT.Sample(samLinear, float2(saturate(dot(N, V)), roughness)).rg;
	float3 SpecularIBL = preFilteredColor * float3(kS * envBRDF.x + envBRDF.y);

	float3 ambient = (diffuseIBL + SpecularIBL) * ao;
	float3 color = ambient + Lo;

	// gamma.
	color = color / (color + 1.0);
	color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0f);
}
