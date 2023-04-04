struct MaterialData
{
	float4 value0; // color, opacity
	float4 value1; // normal, 
	float4 value2; // metallic, roughness, ao, flag
	float4 value3; // customdata
};

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
