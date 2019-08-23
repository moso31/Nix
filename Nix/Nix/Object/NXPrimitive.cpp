#include "NXPrimitive.h"
#include "WICTextureLoader.h"

NXPrimitive::NXPrimitive()
{
}

NXPrimitive::~NXPrimitive()
{
}

void NXPrimitive::Update()
{
	//// Update our time
	//static float t = 0.0f;
	//static ULONGLONG timeStart = 0;
	//ULONGLONG timeCur = GetTickCount64();
	//if (timeStart == 0)
	//	timeStart = timeCur;
	//t = (timeCur - timeStart) / 1000.0f;

	m_pConstantBufferData.world = m_worldMatrix.Transpose();
	g_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &m_pConstantBufferData, 0, 0);

	if (m_pMaterial)
	{
		ConstantBufferMaterial cb;
		cb.material = m_cbDataMaterial.material;
		g_pContext->UpdateSubresource(m_cbMaterial, 0, nullptr, &cb, 0, 0);
	}
}

void NXPrimitive::SetMaterial(const shared_ptr<NXMaterial>& pMaterial)
{
	m_pMaterial = pMaterial;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbMaterial));

	m_cbDataMaterial.material = pMaterial->GetMaterialInfo();
}

void NXPrimitive::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPNT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));

	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferPrimitive);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pConstantBuffer));
	
	NX::ThrowIfFailed(CreateWICTextureFromFile(g_pDevice, L"D:\\rgb.bmp", nullptr, &m_pTextureSRV));
}