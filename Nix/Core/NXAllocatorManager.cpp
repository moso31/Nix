#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"
#include "NXVirtualTextureStreaming.h"

#define Memsize_MB(x) (x * 1024 * 1024)

NXAllocatorManager::NXAllocatorManager()
{
}

NXAllocatorManager::~NXAllocatorManager()
{
}

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();
	m_bRunning.store(true);

	m_pCBAllocator = std::make_unique<CommittedBufferAllocator>(L"CBAllocator", pDevice, true, false, 64u, Memsize_MB(256));
	m_pSBAllocator = std::make_unique<CommittedBufferAllocator>(L"SBAllocator", pDevice, false, false, 64u, Memsize_MB(256));
	m_pRBAllocator = std::make_unique<CommittedBufferAllocator>(L"RBAllocator", pDevice, false, true, 512u, Memsize_MB(256));
	m_pRWBAllocator = std::make_unique<PlacedBufferAllocator>(L"RWBAllocator", pDevice, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, Memsize_MB(256));
	m_pTextureAllocator = std::make_unique<PlacedBufferAllocator>(L"TextureAllocator", pDevice, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, Memsize_MB(2048));

	m_pUpdateSystem = std::make_unique<NXUploadSystem>(pDevice);
	m_pReadbackSystem = std::make_unique<NXReadbackSystem>(pDevice);

	m_pTextureLoader = std::make_unique<NXTextureLoader>();

	m_pSRVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 1000000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pRTVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDSVAllocator = std::make_unique<DescriptorAllocator<false>>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_pNullDescriptorAllocator = std::make_unique<NXNullDescriptor>();
	m_pNullDescriptorAllocator->ExecuteTasks(); // 空描述符 直接单线程执行，且仅执行一次

	m_pShaderVisibleDescAllocator = std::make_unique<DescriptorAllocator<true>>(pDevice, 1000000, 10);


	// 各分配器独立线程，禁止多个Allocator同线程执行，否则很容易死锁。// taskFunction = lambda.
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
	addThread([this]() { m_pReadbackSystem->Update(); }, "NXReadbackSystem\n");
	addThread([this]() { m_pTextureLoader->Update(); }, "NXTextureLoader\n");
	addThread([this]() { m_pCBAllocator->ExecuteTasks(); }, "NXCBAllocator\n");
	addThread([this]() { m_pSBAllocator->ExecuteTasks(); }, "NXSBAllocator\n");
	addThread([this]() { m_pRBAllocator->ExecuteTasks(); }, "NXRBAllocator\n");
	addThread([this]() { m_pRWBAllocator->ExecuteTasks(); }, "NXRWBAllocator\n");
	addThread([this]() { m_pTextureAllocator->ExecuteTasks(); }, "NXTextureAllocator\n");
	addThread([this]() { m_pSRVAllocator->ExecuteTasks(); }, "NXSRVAllocator\n");
	addThread([this]() { m_pRTVAllocator->ExecuteTasks(); }, "NXRTVAllocator\n");
	addThread([this]() { m_pDSVAllocator->ExecuteTasks(); }, "NXDSVAllocator\n");

	// 最后再初始化VT相关的，VT需要用到以上分配器
	m_pVTStreaming = std::make_unique<NXVirtualTextureStreaming>();
	m_pVTStreaming->Init();

	addThread([this]() { 
		m_pVTStreaming->Update(); 
		m_pVTStreaming->ProcessVTBatch();
		}, "NXVirtualTextureStreaming\n");
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
