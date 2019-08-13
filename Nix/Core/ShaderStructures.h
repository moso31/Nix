#pragma once
#include "Header.h"

struct VertexPNT
{
	Vector3 pos;
	Vector3 norm;
	Vector2 tex;
};

struct VertexPNTC
{
	Vector3 pos;
	Vector4 norm;
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
};