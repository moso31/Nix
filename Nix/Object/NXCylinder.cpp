#include "NXCylinder.h"
#include "WICTextureLoader.h"

HRESULT NXCylinder::Init(float radius, float length, int segmentCircle, int segmentLength)
{
	int currVertIdx = 0;

	float yBottom = -length * 0.5f;
	float yTop = length * 0.5f;

	Vector3 nBottom(0.0f, -1.0f, 0.0f);
	Vector3 nTop(0.0f, 1.0f, 0.0f);

	Vector3 pBottom(0.0f, yBottom, 0.0f);
	Vector3 pTop(0.0f, yTop, 0.0f);
	Vector2 uvBottomTop(0.5f, 0.5f);

	m_vertices.push_back({ pBottom, nBottom, uvBottomTop });
	currVertIdx++;
	float segmentCircleInv = 1.0f / (float)segmentCircle;
	for (int i = 0; i < segmentCircle; i++)
	{
		float segNow = (float)i * segmentCircleInv;
		float segNext = (float)(i + 1) * segmentCircleInv;
		float angleNow = segNow * XM_2PI;
		float angleNext = segNext * XM_2PI;

		float xNow = sinf(angleNow);
		float zNow = cosf(angleNow);
		float xNext = sinf(angleNext);
		float zNext = cosf(angleNext);

		Vector3 pNow(xNow * radius, yBottom, zNow * radius);
		Vector3 pNext(xNext * radius, yBottom, zNext * radius);

		Vector2 uvNow((xNow + 1.0f) * 0.5f, (zNow + 1.0f) * 0.5f);
		Vector2 uvNext((xNext + 1.0f) * 0.5f, (zNext + 1.0f) * 0.5f);

		m_vertices.push_back({ pNow, nBottom, uvNow });
		m_vertices.push_back({ pNext, nBottom, uvNext });
		
		m_indices.push_back(0);
		m_indices.push_back(currVertIdx + 1);
		m_indices.push_back(currVertIdx);
		
		currVertIdx += 2;
	}

	float segmentLengthInv = 1.0f / (float)segmentLength;
	for (int i = 0; i < segmentLength; i++)
	{
		float uvDown = (float)i * segmentLengthInv;
		float uvUp = (float)(i + 1) * segmentLengthInv;
		float yDown = uvDown * length + yBottom;
		float yUp = uvUp * length + yBottom;

		for (int j = 0; j < segmentCircle; j++)
		{
			float segNow = (float)j * segmentCircleInv;
			float segNext = (float)(j + 1) * segmentCircleInv;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;

			float xNow = sinf(angleNow);
			float zNow = cosf(angleNow);
			float xNext = sinf(angleNext);
			float zNext = cosf(angleNext);

			Vector3 pNowUp = { xNow * radius, yUp, zNow * radius };
			Vector3 pNextUp = { xNext * radius, yUp, zNext * radius };
			Vector3 pNowDown = { xNow * radius, yDown, zNow * radius };
			Vector3 pNextDown = { xNext * radius, yDown, zNext * radius };

			Vector3 nNow = { xNow, 0.0f, zNow };
			Vector3 nNext = { xNext, 0.0f, zNext };

			Vector2 uvNowUp		= { segNow,		uvUp };
			Vector2 uvNextUp	= { segNext,	uvUp };
			Vector2 uvNowDown	= { segNow,		uvDown };
			Vector2 uvNextDown	= { segNext,	uvDown };

			m_vertices.push_back({ pNowUp,		nNow,	uvNowUp });
			m_vertices.push_back({ pNextUp,		nNext,	uvNextUp });
			m_vertices.push_back({ pNowDown,	nNow,	uvNowDown });
			m_vertices.push_back({ pNextDown,	nNext,	uvNextDown });

			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	m_vertices.push_back({ pTop, nTop, uvBottomTop });
	int SaveIdx = currVertIdx++;
	for (int i = 0; i < segmentCircle; i++)
	{
		float segNow = (float)i * segmentCircleInv;
		float segNext = (float)(i + 1) * segmentCircleInv;
		float angleNow = segNow * XM_2PI;
		float angleNext = segNext * XM_2PI;

		float xNow = sinf(angleNow);
		float zNow = cosf(angleNow);
		float xNext = sinf(angleNext);
		float zNext = cosf(angleNext);

		Vector3 pNow(xNow * radius, yTop, zNow * radius);
		Vector3 pNext(xNext * radius, yTop, zNext * radius);

		Vector2 uvNow((xNow + 1.0f) * 0.5f, (zNow + 1.0f) * 0.5f);
		Vector2 uvNext((xNext + 1.0f) * 0.5f, (zNext + 1.0f) * 0.5f);

		m_vertices.push_back({ pNow, nTop, uvNow });
		m_vertices.push_back({ pNext, nTop, uvNext });
		
		m_indices.push_back(SaveIdx);
		m_indices.push_back(currVertIdx);
		m_indices.push_back(currVertIdx + 1);
		
		currVertIdx += 2;
	}

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
	return S_OK;
}

void NXCylinder::Update()
{
	// Update our time
	static float t = 0.0f;
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0)
		timeStart = timeCur;
	t = (timeCur - timeStart) / 1000.0f;

	m_pConstantBufferData.world = Matrix::CreateRotationX(t);

	ConstantBufferPrimitive cb;
	cb.world = m_pConstantBufferData.world.Transpose();
	g_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);
}

void NXCylinder::Render()
{
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShaderResources(0, 1, &m_pTextureSRV);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXCylinder::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)			m_pTextureSRV->Release();
}
