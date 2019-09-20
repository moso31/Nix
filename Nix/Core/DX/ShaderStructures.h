#pragma once
#include "Header.h"
#include "NXLight.h"

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

struct ConstantBufferShadowMapCamera
{
	Matrix view;
	Matrix projection;
};

struct ConstantBufferMaterial
{
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular; // w = SpecPower
	Vector4 reflect;
	float opacity;
	Vector3 _align16;
};

struct ConstantBufferLight
{
	DirectionalLightInfo dirLight;
	PointLightInfo pointLight;
	SpotLightInfo spotLight;
};