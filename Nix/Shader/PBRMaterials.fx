struct PBRMaterialStandard
{
	float3 albedo;
	float _0;
	float3 normal;
	float metallic;
	float roughness;
	float ao;
	float2 _1;
};

struct PBRMaterialTranslucent
{
	float3 albedo;
	float opacity;
	float3 normal;
	float metallic;
	float roughness;
	float ao;
	float2 _1;
};
