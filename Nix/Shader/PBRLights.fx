#define NUM_DISTANT_LIGHT 4
#define NUM_POINT_LIGHT 16
#define NUM_SPOT_LIGHT 16

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
	float innerAngle;
	float3 direction;
	float outerAngle;
	float3 color;
	float intensity;
};
