#pragma once
#include "Header.h"
#include "NXMaterial.h"
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

struct ConstantBufferMaterial
{
	MaterialInfo material;
};

struct ConstantBufferLight
{
	DirectionalLightInfo dirLight;
	PointLightInfo pointLight;
	SpotLightInfo spotLight;
};