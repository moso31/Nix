#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();

	m_pCBAllocator = new CommittedBufferAllocator(pDevice);
	m_pSBAllocator = new CommittedBufferAllocator(pDevice);
	m_pTextureAllocator = new PlacedBufferAllocator(pDevice);

	m_pUpdateSystem = new UploadSystem(pDevice);

	m_pSRVAllocator = new ShaderVisibleDescriptorAllocator(pDevice, 1000000);
	m_pRTVAllocator = new NonVisibleDescriptorAllocator(pDevice, 4096);
	m_pDSVAllocator = new NonVisibleDescriptorAllocator(pDevice, 4096);
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
}
