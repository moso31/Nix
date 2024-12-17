#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();

	m_pCBAllocator = new CommittedBufferAllocator(pDevice, true);
	m_pSBAllocator = new CommittedBufferAllocator(pDevice, false);
	m_pTextureAllocator = new PlacedBufferAllocator(pDevice, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	m_pUpdateSystem = new UploadSystem(pDevice);
	m_pTextureLoader = new NXTextureLoader();

	m_pSRVAllocator = new DescriptorAllocator<false>(pDevice, 1000000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pRTVAllocator = new DescriptorAllocator<false>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDSVAllocator = new DescriptorAllocator<false>(pDevice, 4096, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_pShaderVisibleDescAllocator = new DescriptorAllocator<true>(pDevice, 1000000, 10);

	// 使用独立线程，不能和其它分配器混用
	// 一旦混用可能会从其它分配器回调这里的方法-诱发死锁。

	std::thread([this]() {
		NXPrint::Write(0, "NXUploadSystem\n");
		while (true)
		{
			m_pUpdateSystem->Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXTextureLoader\n");
		while (true)
		{
			m_pTextureLoader->Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXCBAllocator\n");
		while (true)
		{
			m_pCBAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXSBAllocator\n");
		while (true)
		{
			m_pSBAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXTextureAllocator\n");
		while (true)
		{
			m_pTextureAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXSRVAllocator\n");
		while (true)
		{
			m_pSRVAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXRTVAllocator\n");
		while (true)
		{
			m_pRTVAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();

	std::thread([this]() {
		NXPrint::Write(0, "NXDSVAllocator\n");
		while (true)
		{
			m_pDSVAllocator->ExecuteTasks();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}).detach();
}

void NXAllocatorManager::Update()
{
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
