struct DistantLight
{
	float3 direction;
	float _0;
	float3 color;
	float illuminance;
};

struct PointLight
{
	float3 position;
	float _0;
	float3 color;
	float intensity;
};

struct SpotLight
{
	float3 position;
	float _0;
	float3 direction;
	float _1;
	float3 color;
	float _2;
};
