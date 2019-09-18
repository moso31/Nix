#include "NXPlane.h"

NXPlane::NXPlane() :
	m_width(0.0f),
	m_height(0.0f)
{
}

void NXPlane::Init(float width, float height)
{
	float x = width * 0.5f, z = height * 0.5f;
	// Create vertex buffer
	m_vertices =
	{
		// +Y
		{ Vector3(-x, 0.0f, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(+x, 0.0f, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(+x, 0.0f, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(-x, 0.0f, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 0.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitVertexIndexBuffer();
	InitAABB();
}
