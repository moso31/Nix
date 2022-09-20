#include "GlobalBufferManager.h"

ComPtr<ID3D11Buffer>				NXGlobalBufferManager::m_cbObject;
ConstantBufferObject				NXGlobalBufferManager::m_cbDataObject;
ComPtr<ID3D11Buffer>				NXGlobalBufferManager::m_cbCamera;
ConstantBufferCamera				NXGlobalBufferManager::m_cbDataCamera;
ComPtr<ID3D11Buffer>				NXGlobalBufferManager::m_cbShadowTest;
ConstantBufferShadowTest			NXGlobalBufferManager::m_cbDataShadowTest;

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

	bufferDesc.ByteWidth = sizeof(ConstantBufferShadowTest);
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbShadowTest));
}

D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutP[1];
D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPT[2];
D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNT[3];
D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNTT[4];
D3D11_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutEditorObject[2];

void NXGlobalInputLayout::Init()
{
	layoutP[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	layoutPT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPT[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	layoutPNT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	layoutPNTT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNTT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNTT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutPNTT[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	layoutEditorObject[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layoutEditorObject[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
}
