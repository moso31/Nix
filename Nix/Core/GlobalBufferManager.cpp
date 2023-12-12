#include "GlobalBufferManager.h"
#include "BaseDefs/NixCore.h"
#include "Global.h"

ComPtr<ID3D12Resource>				NXGlobalBufferManager::m_cbObject;
ConstantBufferObject				NXGlobalBufferManager::m_cbDataObject;
ComPtr<ID3D12Resource>				NXGlobalBufferManager::m_cbCamera;
ConstantBufferCamera				NXGlobalBufferManager::m_cbDataCamera;
ComPtr<ID3D12Resource>				NXGlobalBufferManager::m_cbShadowTest;
ConstantBufferShadowTest			NXGlobalBufferManager::m_cbDataShadowTest;

void NXGlobalBufferManager::Init()
{
	m_cbObject = NX12Util::CreateBuffer(g_pDevice.Get(), L"CB Object", sizeof(ConstantBufferObject), D3D12_HEAP_TYPE_UPLOAD);
	m_cbCamera = NX12Util::CreateBuffer(g_pDevice.Get(), L"CB Camera", sizeof(ConstantBufferCamera), D3D12_HEAP_TYPE_UPLOAD);
	m_cbShadowTest = NX12Util::CreateBuffer(g_pDevice.Get(), L"CB Shadow Test", sizeof(ConstantBufferShadowTest), D3D12_HEAP_TYPE_UPLOAD);
}

D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutP[1];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPT[2];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNT[3];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNTT[4];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutEditorObject[2];

void NXGlobalInputLayout::Init()
{
	layoutP[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPT[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPNT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPNTT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutEditorObject[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutEditorObject[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
}
