#pragma once
#include "Header.h"

struct VertexP
{
	VertexP() = default;
	VertexP(const Vector3& pos) : pos(pos) {}
	Vector3 pos;
};

struct VertexPT
{
	VertexPT() = default;
	VertexPT(const Vector3& pos, const Vector2& tex) :
		pos(pos), tex(tex) {}
	Vector3 pos;
	Vector2 tex;
};

struct VertexPNT
{
	VertexPNT() = default;
	VertexPNT(const Vector3& pos, const Vector3& norm, const Vector2& tex) : 
		pos(pos), norm(norm), tex(tex) {}
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
};

struct VertexPNTC
{
	VertexPNTC(const Vector3& pos, const Vector3& norm, const Vector2& tex, const Vector4& color) : 
		pos(pos), norm(norm), tex(tex), color(color) {}
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
	Vector4 color;
};

struct VertexPNTT
{
	VertexPNTT() = default;
	VertexPNTT(const Vector3& pos, const Vector3& norm, const Vector2& tex, const Vector3& tangent) :
		pos(pos), norm(norm), tex(tex), tangent(tangent) {}
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
	Vector3 tangent;
};

struct VertexEditorObjects
{
	VertexEditorObjects() = default;
	VertexEditorObjects(const Vector3& pos, const Vector4& color) :
		pos(pos), color(color) {}
	Vector3 pos;
	Vector4 color;
};

struct ConstantBufferFloat
{
	float value;
	Vector3 _0;
};

struct ConstantBufferVector2
{
	Vector2 value;
	Vector2 _0;
};

struct ConstantBufferVector3
{
	Vector3 value;
	float _0;
};

struct ConstantBufferVector4
{
	Vector4 value;
};

struct ConstantBufferGlobalData
{
	float time;
	Vector3 _0;
};

struct ConstantBufferObject
{
	Matrix world;
	Matrix worldInverseTranspose;
	Matrix view;
	Matrix viewInverse;
	Matrix viewTranspose;
	Matrix viewInverseTranspose;

	Matrix worldView;
	Matrix worldViewInverseTranspose;
	Matrix projection;

	ConstantBufferGlobalData globalData;
};

struct ConstantBufferShadowTest
{
	Matrix view[8];
	Matrix projection[8];

	// 记录 CSM 各级 用于计算Transition的过渡 的信息。
	Vector4 frustumParams[8]; // x: frustum far; y : transition length

	float cascadeCount;
	float shadowDistance;
	float cascadeTransitionScale;
	int depthBias;

	float test_transition;
	Vector3 _0;
};

struct ConstantBufferDistantLight
{
	Vector3 direction;
	float _0;
	Vector3 color;
	float illuminance;
};

struct ConstantBufferPointLight
{
	Vector3 position;
	float influenceRadius;
	Vector3 color;
	float intensity;
};

struct ConstantBufferSpotLight
{
	Vector3 position;
	float innerAngle;
	Vector3 direction;
	float outerAngle;
	Vector3 color;
	float intensity;
	float influenceRadius;
	Vector3 _0;
};

struct ConstantBufferLight
{
	ConstantBufferDistantLight distantLight[4];
	ConstantBufferPointLight pointLight[16];
	ConstantBufferSpotLight spotLight[16];
};

struct ConstantBufferCamera
{
	Vector4 Params0;
	Vector4 Params1;
	Vector4 Params2;
};