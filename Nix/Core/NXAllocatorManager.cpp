#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();
	m_bRunning.store(true);

	m_pCBAllocator = std::make_unique<CommittedBufferAllocator>(pDevice, true, 64u, 0x10000000u);
	m_pSBAllocator = std::make_unique<CommittedBufferAllocator>(pDevice, false, 64u, 0x10000000u);
	m_pTextureAllocator = std::make_unique<PlacedBufferAllocator>(pDevice, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 0x10000000u);

	m_pUpdateSystem = std::make_unique<UploadSystem>(pDevice);
	m_pTextureLoader = std::make_unique<NXTextureLoader>();

	m_pSRVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 1000000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pRTVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDSVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_pShaderVisibleDescAllocator = std::make_unique<DescriptorAllocator<true>>(pDevice, 1000000, 10);

	// 各分配器独立线程，不能和其它分配器混用 // 混用=死锁。// taskFunction = lambda.
	auto addThread = [this](auto taskFunction, const char* name, int sleepMS = 1) {
		std::thread t([this, taskFunction, name, sleepMS]() {
			NXPrint::Write(0, name);
			while (m_bRunning)
			{
				taskFunction();
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
			}
		});

		m_threads.push_back(std::move(t));
	};

	addThread([this]() { m_pUpdateSystem->Update(); }, "NXUploadSystem\n");
	addThread([this]() { m_pTextureLoader->Update(); }, "NXTextureLoader\n");
	addThread([this]() { m_pCBAllocator->ExecuteTasks(); }, "NXCBAllocator\n");
	addThread([this]() { m_pSBAllocator->ExecuteTasks(); }, "NXSBAllocator\n");
	addThread([this]() { m_pTextureAllocator->ExecuteTasks(); }, "NXTextureAllocator\n");
	addThread([this]() { m_pSRVAllocator->ExecuteTasks(); }, "NXSRVAllocator\n");
	addThread([this]() { m_pRTVAllocator->ExecuteTasks(); }, "NXRTVAllocator\n");
	addThread([this]() { m_pDSVAllocator->ExecuteTasks(); }, "NXDSVAllocator\n");
}

void NXAllocatorManager::Update()
{
}

void NXAllocatorManager::Release()
{
	m_bRunning.store(false);
	for (auto& t : m_threads)
	{
		if (t.joinable())
			t.join();
	}
}
