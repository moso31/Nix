#ifndef _COMMON_
#define _COMMON_

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
	float4 cameraParams0;
	float4 cameraParams1; // n, f, f / (f - n), -f * n / (f - n)
	float4 cameraParams2; // proj._11, proj._22, invProj._11, invProj._22
}

#endif // !_COMMON_