#pragma once
#include "Header.h"
#include "Material.h"
#include "Light.h"

struct VertexPNT
{
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
	Matrix worldInvTranspose;
};

struct ConstantBufferCamera
{
	Matrix view;
	Matrix projection;
};

struct ConstantBufferMaterial
{
	MaterialInfo material;
};

struct ConstantBufferLight
{
	LightInfo light;
};