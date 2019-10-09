struct DirectionalLight
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 direction;
	float _align16;
};

struct PointLight
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 position;
	float range;
	float3 att;
	float _align16;
};

struct SpotLight
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 position;
	float range;
	float3 direction;
	float spot;
	float3 att;
	float _align16;
};

struct Material
{
	float4 ambient;
	float4 diffuse;
	float4 specular; // w = SpecPower
	float4 reflect;
	float opacity;
	float3 _align16;
};

void ComputeDirectionalLight(Material mat, DirectionalLight L,
	float3 normal, float3 toEye,
	out float4 out_ambient,
	out float4 out_diffuse,
	out float4 out_specular)
{
	out_ambient = 0;
	out_diffuse = 0;
	out_specular = 0;

	out_ambient = mat.ambient * L.ambient;
	float kDiff = dot(-L.direction, normal);

	// Flatten to avoid dynamic branching.
	[flatten]
	if (kDiff > 0.0f)
	{
		float3 v = reflect(L.direction, normal);
		float kSpec = pow(max(dot(v, toEye), 0.0f), mat.specular.w);

		out_diffuse = kDiff * mat.diffuse * L.diffuse;
		out_specular = kSpec * mat.specular * L.specular;
	}
}

void ComputePointLight(Material mat, PointLight L,
	float3 pos, float3 normal, float3 toEye,
	out float4 out_ambient,
	out float4 out_diffuse,
	out float4 out_specular)
{
	out_ambient = 0;
	out_diffuse = 0;
	out_specular = 0;

	float3 lightDir = pos - L.position;
	float d = length(lightDir);
	if (d > L.range)
		return;

	lightDir *= rcp(d);

	out_ambient = mat.ambient * L.ambient;
	float kDiff = dot(-lightDir, normal);

	[flatten]
	if (kDiff > 0.0f)
	{
		float3 v = reflect(lightDir, normal);
		float kSpec = pow(max(dot(v, toEye), 0.0f), mat.specular.w);
		float att = rcp(L.att.x + L.att.y * d + L.att.z * d * d);

		out_diffuse = att * kDiff * mat.diffuse * L.diffuse;
		out_specular = att * kSpec * mat.specular * L.specular;
	}
}


void ComputeSpotLight(Material mat, SpotLight L,
	float3 pos, float3 normal, float3 toEye,
	out float4 out_ambient,
	out float4 out_diffuse,
	out float4 out_specular)
{
	out_ambient = 0;
	out_diffuse = 0;
	out_specular = 0;

	float3 lightDir = pos - L.position;
	float d = length(lightDir);
	if (d > L.range)
		return;

	lightDir *= rcp(d);

	out_ambient = mat.ambient * L.ambient;
	float kDiff = dot(-lightDir, normal);

	[flatten]
	if (kDiff > 0.0f)
	{
		float3 v = reflect(lightDir, normal);
		float kSpec = pow(max(dot(v, toEye), 0.0f), mat.specular.w);
		float att = rcp(L.att.x + L.att.y * d + L.att.z * d * d);

		out_diffuse = att * kDiff * mat.diffuse * L.diffuse;
		out_specular = att * kSpec * mat.specular * L.specular;
	}

	float kSpot = pow(max(dot(L.direction, lightDir), 0), L.spot);
	out_ambient *= kSpot;
	out_diffuse *= kSpot;
	out_specular *= kSpot;
}

// Shadow map

static const float SHADOWMAP_SIZE = 2048.0f;
static const float SHADOWMAP_INVSIZE = 1.0f / SHADOWMAP_SIZE;

static const float2 offset3x3[9] =
{
	float2(-1, -1), float2(0, -1), float2(1, -1),
	float2(-1,  0), float2(0,  0), float2(1,  0),
	float2(-1, +1), float2(0, +1), float2(1, +1)
};

float ShadowMapFilter(SamplerComparisonState samp, Texture2D tex, float4 pos)
{
	// Complete projection by doing division by w.
	pos.xyz /= pos.w;
	
	const float invSize = SHADOWMAP_INVSIZE;
	float percentLit = 0.0f;

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		// 对ShadowMap进行3x3范围采样
		float offset = offset3x3[i] * 0.0f * SHADOWMAP_INVSIZE;
		// 采用的比较是less，即：如果shadowMap的depth < 当前渲染坐标的depth，就计入percentLit。
		percentLit += tex.SampleCmpLevelZero(samp, pos.xy + offset, pos.z).r;
	}

	return percentLit /= 9.0f;
}