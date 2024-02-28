#include "NXAllocatorManager.h"

void NXAllocatorManager::Init(ID3D12Device* pDevice)
{
	m_pCBufferAllocator = new CommittedAllocator(pDevice, 256);
	m_pTextureAllocator = new PlacedAllocator(pDevice, 256);
	m_pDescriptorAllocator = new DescriptorAllocator(pDevice);
	m_pRTVAllocator = new RTVAllocator(pDevice);
	m_pDSVAllocator = new DSVAllocator(pDevice);

	m_pShaderVisibleDescriptorHeap = new NXShaderVisibleDescriptorHeap(pDevice);

	m_pCommandList = NX12Util::CreateCommandList(pDevice, m_pCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = NX12Util::CreateCommandQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, false);
	m_pCommandAllocator = NX12Util::CreateCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void NXAllocatorManager::Release()
{
	m_pCommandList->Release();
	m_pCommandQueue->Release();
	m_pCommandAllocator->Release();

	delete m_pCBufferAllocator;
	delete m_pTextureAllocator;
	delete m_pDescriptorAllocator;
	delete m_pRTVAllocator;
	delete m_pDSVAllocator;

	delete m_pShaderVisibleDescriptorHeap;
}
