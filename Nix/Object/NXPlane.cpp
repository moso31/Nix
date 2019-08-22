#include "NXPlane.h"
#include "WICTextureLoader.h"

HRESULT NXPlane::Init(float width, float height)
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

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPNT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();

	HRESULT hr;
	hr = g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();

	g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferPrimitive);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	hr = g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	hr = CreateWICTextureFromFile(g_pDevice, L"D:\\rgb.bmp", nullptr, &m_pTextureSRV);
	if (FAILED(hr))
		return hr;

	m_pConstantBufferData.world = Matrix::Identity();
	m_pConstantBufferData.worldInvTranspose = Matrix::Identity();

	return S_OK;
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
