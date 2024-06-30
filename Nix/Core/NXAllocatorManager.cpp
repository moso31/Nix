#include "NXAllocatorManager.h"
#include "NXGlobalDefinitions.h"

void NXAllocatorManager::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();
	m_pCBufferAllocator = new CommittedAllocator(pDevice);
	m_pTextureAllocator = new PlacedAllocator(pDevice);
	m_pDescriptorAllocator = new DescriptorAllocator(pDevice);
	m_pRTVAllocator = new RTVAllocator(pDevice);
	m_pDSVAllocator = new DSVAllocator(pDevice);

	// Shader visible heap 预留10个静态描述符。目前这么做的原因暂时只有一个：imgui
	// 在imgui DX12中，必须使用描述符堆中固定位置的索引，作为字体纹理的描述符堆
	m_pShaderVisibleDescriptorHeap = new NXShaderVisibleDescriptorHeap(pDevice, 10);
	m_pShaderVisibleDescriptorHeap->GetHeap()->SetName(L"NXGlobal shader visible heap");
}

void NXAllocatorManager::Release()
{
	delete m_pCBufferAllocator;
	delete m_pTextureAllocator;
	delete m_pDescriptorAllocator;
	delete m_pRTVAllocator;
	delete m_pDSVAllocator;

	delete m_pShaderVisibleDescriptorHeap;
}
