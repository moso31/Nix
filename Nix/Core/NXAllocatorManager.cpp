#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();

	m_pCBAllocator = new CommittedBufferAllocator(pDevice);
	m_pSBAllocator = new CommittedBufferAllocator(pDevice);
	m_pTextureAllocator = new PlacedBufferAllocator(pDevice, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	m_pUpdateSystem = new UploadSystem(pDevice);

	m_pSRVAllocator = new DescriptorAllocator<false>(pDevice, 1000000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pRTVAllocator = new DescriptorAllocator<false>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDSVAllocator = new DescriptorAllocator<false>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_pShaderVisibleDescAllocator = new DescriptorAllocator<true>(pDevice, 1000000, 10);

	std::thread([this]() {
		while (true)
		{
			Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();
}

void NXAllocatorManager::Update()
{
	m_pUpdateSystem->Update();

	m_pCBAllocator->ExecuteTasks();
	m_pSBAllocator->ExecuteTasks();
	m_pTextureAllocator->ExecuteTasks();
	m_pSRVAllocator->ExecuteTasks();
	m_pRTVAllocator->ExecuteTasks();
	m_pDSVAllocator->ExecuteTasks();
}

void NXAllocatorManager::Release()
{
	SafeDelete(m_pCBAllocator);
	SafeDelete(m_pSBAllocator);
	SafeDelete(m_pTextureAllocator);
	SafeDelete(m_pUpdateSystem);
	SafeDelete(m_pSRVAllocator);
	SafeDelete(m_pRTVAllocator);
	SafeDelete(m_pDSVAllocator);

	SafeDelete(m_pShaderVisibleDescAllocator);
}
