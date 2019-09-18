#include "NXBox.h"

NXBox::NXBox()
{
}

void NXBox::Init(float x, float y, float z)
{
	x *= 0.5f;
	y *= 0.5f;
	z *= 0.5f;

	m_vertices =
	{
		// -X
		{ Vector3(-x, +y, +z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(-x, +y, -z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(-x, -y, -z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(-x, -y, +z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f) },

		// +X
		{ Vector3(+x, +y, -z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(+x, +y, +z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(+x, -y, +z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(+x, -y, -z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f) },

		// -Y
		{ Vector3(-x, -y, -z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(+x, -y, -z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(+x, -y, +z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(-x, -y, +z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 0.0f) },

		// +Y
		{ Vector3(-x, +y, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(+x, +y, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(+x, +y, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(-x, +y, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 0.0f) },

		// -Z
		{ Vector3(-x, +y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(+x, +y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(+x, -y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(-x, -y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 0.0f) },

		// +Z
		{ Vector3(+x, +y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(-x, +y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 1.0f) },
		{ Vector3(-x, -y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(+x, -y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 0.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3,

		4,  5,  6,
		4,  6,  7,

		8,  9,  10,
		8,  10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23
	};

	InitVertexIndexBuffer();
	InitAABB();
}

void NXBox::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();
}
