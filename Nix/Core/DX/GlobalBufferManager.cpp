#include "GlobalBufferManager.h"

ID3D11Buffer*						NXGlobalBufferManager::m_cbWorld;
ConstantBufferPrimitive				NXGlobalBufferManager::m_cbDataWorld;
ID3D11Buffer*						NXGlobalBufferManager::m_cbCamera;
ConstantBufferCamera				NXGlobalBufferManager::m_cbDataCamera;
ID3D11Buffer*						NXGlobalBufferManager::m_cbShadowMap;
ConstantBufferShadowMapTransform	NXGlobalBufferManager::m_cbDataShadowMap;

NXGlobalBufferManager::NXGlobalBufferManager()
{
}

NXGlobalBufferManager::~NXGlobalBufferManager()
{
}

void NXGlobalBufferManager::Init()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferPrimitive);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbWorld));

	bufferDesc.ByteWidth = sizeof(ConstantBufferCamera);
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbCamera));
}
