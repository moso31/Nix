#include "GlobalBufferManager.h"
#include "BaseDefs/NixCore.h"
#include "Global.h"
#include "NXAllocatorManager.h"

MultiFrame<CommittedResourceData<ConstantBufferObject>>		NXGlobalBufferManager::m_cbDataObject;
MultiFrame<CommittedResourceData<ConstantBufferCamera>>		NXGlobalBufferManager::m_cbDataCamera;
MultiFrame<CommittedResourceData<ConstantBufferShadowTest>>	NXGlobalBufferManager::m_cbDataShadowTest;

void NXGlobalBufferManager::Init()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbDataObject.Get(i));
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbDataCamera.Get(i));
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbDataShadowTest.Get(i));
	}
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
