#pragma once
#include "Header.h"

struct VertexP
{
	VertexP() = default;
	VertexP(Vector3 pos) : pos(pos) {}
	Vector3 pos;
};

struct VertexPNT
{
	VertexPNT() = default;
	VertexPNT(Vector3 pos, Vector3 norm, Vector2 tex) : 
		pos(pos), norm(norm), tex(tex) {}
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
};

struct VertexPNTC
{
	VertexPNTC(Vector3 pos, Vector3 norm, Vector2 tex, Vector4 color) : 
		pos(pos), norm(norm), tex(tex), color(color) {}
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
	Vector4 color;
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
	float metallic;
	float roughness;
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