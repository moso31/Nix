#include "NXPlane.h"

NXPlane::NXPlane() :
	m_width(0.0f),
	m_height(0.0f)
{
}

void NXPlane::Init(float width, float height, NXPlaneAxis Axis)
{
	float w = width * 0.5f, h = height * 0.5f;
	// Create vertex buffer
	switch (Axis)
	{
	case POSITIVE_X:
		m_vertices =
		{
			{ Vector3(0.0f, -w, -h), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f) },
			{ Vector3(0.0f, +w, -h), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(0.0f, +w, +h), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(0.0f, -w, +h), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		};
		break;
	case POSITIVE_Y:
		m_vertices =
		{
			{ Vector3(-w, 0.0f, +h), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
			{ Vector3(+w, 0.0f, +h), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(+w, 0.0f, -h), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(-w, 0.0f, -h), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 0.0f) },
		};
		break;
	case POSITIVE_Z:
		m_vertices =
		{
			{ Vector3(-w, -h, 0.0f), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 0.0f) },
			{ Vector3(+w, -h, 0.0f), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(+w, +h, 0.0f), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(-w, +h, 0.0f), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 1.0f) },
		};
		break;
	case NEGATIVE_X:
		m_vertices =
		{
			{ Vector3(0.0f, -w, +h), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f) },
			{ Vector3(0.0f, +w, +h), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(0.0f, +w, -h), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(0.0f, -w, -h), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f) },
		};
		break;
	case NEGATIVE_Y:
		m_vertices =
		{
			{ Vector3(-w, 0.0f, -h), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 0.0f) },
			{ Vector3(+w, 0.0f, -h), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(+w, 0.0f, +h), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(-w, 0.0f, +h), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		};
		break;
	case NEGATIVE_Z:
		m_vertices =
		{
			{ Vector3(-w, +h, 0.0f), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 1.0f) },
			{ Vector3(+w, +h, 0.0f), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 1.0f) },
			{ Vector3(+w, -h, 0.0f), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 0.0f) },
			{ Vector3(-w, -h, 0.0f), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 0.0f) },
		};
		break;
	}

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitVertexIndexBuffer();
	InitAABB();
}
