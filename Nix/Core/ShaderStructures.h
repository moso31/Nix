#pragma once
#include "Header.h"

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

struct ConstantBufferPrimitive
{
	Matrix world;
};

struct ConstantBufferCamera
{
	Matrix view;
	Matrix projection;
	Vector3 eyePosition;
	float _align16;
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
	Vector3 _align16;
};

struct ConstantBufferDistantLight
{
	Vector3 direction;
	Vector3 color;
	Vector2 _align16;
};

struct ConstantBufferPointLight
{
	Vector3 position;
	Vector3 color;
	Vector2 _align16;
};

struct ConstantBufferSpotLight
{
	Vector3 position;
	Vector3 direction;
	Vector3 color;
	Vector3 _align16;
};

struct ConstantBufferLight
{
	ConstantBufferPointLight pointLight;
};