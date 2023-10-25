#ifndef _COMMON_
#define _COMMON_

struct NXGBufferParams
{
	float3 albedo;
	float metallic;
	float3 normal;
	float roughness;
	float ao;
	float3 _0;
};

struct NXCBGlobalData
{
	float Time;
	float3 _0;
};

cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_viewInverse;
	matrix m_viewTranspose;
	matrix m_viewInverseTranspose;
	matrix m_worldView;
	matrix m_worldViewInverseTranspose;
	matrix m_projection;
	matrix m_projectionInv;
	NXCBGlobalData g;
}

cbuffer ConstantBufferCamera : register(b1)
{
	// xy: RTSize, zw: 1.0f / RTSize
	float4 cameraParams0;

	// n, f, f / (f - n), -f * n / (f - n)
	// Nix use left-hand space, so 
	// PosCS.z = PosVS.z * cameraParams1.z + cameraParams1.w, PosCS.w = PosVS.z
	// PosNDC.z = PosCS.z / PosCS.w
	float4 cameraParams1; 

	// proj._11, proj._22, invProj._11, invProj._22
	float4 cameraParams2; 
}

#endif // !_COMMON_