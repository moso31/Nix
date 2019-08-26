#include "NXPlane.h"
#include "WICTextureLoader.h"

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
}

void NXPlane::Render()
{
	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShaderResources(0, 1, &m_pTextureSRV);
	g_pContext->PSSetConstantBuffers(3, 1, &m_cbMaterial);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXPlane::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();
}
