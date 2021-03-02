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

struct ConstantBufferFloat
{
	float Value;
	Vector3 _0;
};

struct ConstantBufferVector2
{
	Vector2 Value;
	Vector2 _0;
};

struct ConstantBufferVector3
{
	float _0;
	Vector3 Value;
};

struct ConstantBufferVector4
{
	Vector4 Value;
};

struct ConstantBufferObject
{
	Matrix world;
	Matrix worldInverseTranspose;
	Matrix view;
	Matrix projection;
};

struct ConstantBufferCamera
{
	Vector3 eyePosition;
	float _0;
};

struct ConstantBufferShadowMapTransform
{
	Matrix view;
	Matrix projection;
	Matrix texture;
};

struct ConstantBufferMaterial
{
	Vector3 albedo;
	Vector3 normal;
	float metallic;
	float roughness;
	float ao;
	Vector3 _0;
};

struct ConstantBufferDistantLight
{
	Vector3 direction;
	float _0;
	Vector3 color;
	float _1;
};

struct ConstantBufferPointLight
{
	Vector3 position;
	float _0;
	Vector3 color;
	float _1;
};

struct ConstantBufferSpotLight
{
	Vector3 position;
	float _0;
	Vector3 direction;
	float _1;
	Vector3 color;
	float _2;
};

struct ConstantBufferLight
{
	ConstantBufferPointLight pointLight;
};