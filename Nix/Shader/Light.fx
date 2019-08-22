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