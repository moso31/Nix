#include "NXPlane.h"
#include "WICTextureLoader.h"

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

void NXPlane::Update()
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

void NXPlane::Render()
{
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
