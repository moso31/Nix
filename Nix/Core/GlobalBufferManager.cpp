#include "GlobalBufferManager.h"

ID3D11Buffer*						NXGlobalBufferManager::m_cbObject;
ConstantBufferObject				NXGlobalBufferManager::m_cbDataObject;
ID3D11Buffer*						NXGlobalBufferManager::m_cbCamera;
ConstantBufferCamera				NXGlobalBufferManager::m_cbDataCamera;
ID3D11Buffer*						NXGlobalBufferManager::m_cbShadowMap;
ConstantBufferShadowMapTransform	NXGlobalBufferManager::m_cbDataShadowMap;

void NXGlobalBufferManager::Init()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbObject));

	bufferDesc.ByteWidth = sizeof(ConstantBufferCamera);
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbCamera));
}

D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutP[1];
D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNT[3];

void NXGlobalInputLayout::Init()
{
	layoutP[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	layoutPNT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 };
}
