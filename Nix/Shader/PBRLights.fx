struct DirectionalLight
{
	float3 position;
	float _0;
	float3 color;
	float _1;
};

struct PointLight
{
	float3 position;
	float _0;
	float3 color;
	float _1;
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
