#include "NXBox.h"

void NXBox::Init(float x, float y, float z)
{
	x *= 0.5f;
	y *= 0.5f;
	z *= 0.5f;
	// Create vertex buffer
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
}

void NXBox::Update()
{
	// Update our time
	static float t = 0.0f;
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0)
		timeStart = timeCur;
	t = (timeCur - timeStart) / 1000.0f;

	Matrix mxWorld = Matrix::CreateRotationY(t * 0.5f);
	m_pConstantBufferData.world = mxWorld;
	m_pConstantBufferData.worldInvTranspose = mxWorld.Invert().Transpose();

	ConstantBufferPrimitive cb;
	cb.world = m_pConstantBufferData.world.Transpose();
	cb.worldInvTranspose = m_pConstantBufferData.worldInvTranspose.Transpose();
	g_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	if (m_pMaterial)
	{
		ConstantBufferMaterial cb;
		cb.material = m_cbDataMaterial.material;
		g_pContext->UpdateSubresource(m_cbMaterial, 0, nullptr, &cb, 0, 0);
	}
}

void NXBox::Render()
{
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShaderResources(0, 1, &m_pTextureSRV);
	g_pContext->PSSetConstantBuffers(3, 1, &m_cbMaterial);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXBox::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();
}
