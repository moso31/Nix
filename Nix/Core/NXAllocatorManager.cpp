#include "NXAllocatorManager.h"

void NXAllocatorManager::Init()
{
	m_pCBufferAllocator = new CommittedAllocator(m_pDevice, 256);
	m_pTextureAllocator = new PlacedAllocator(m_pDevice, 256);
	m_pDescriptorAllocator = new DescriptorAllocator(m_pDevice);
	m_pRTVAllocator = new RTVAllocator(m_pDevice);
	m_pDSVAllocator = new DSVAllocator(m_pDevice);
}

void NXAllocatorManager::Release()
{
	delete m_pCBufferAllocator;
	delete m_pTextureAllocator;
	delete m_pDescriptorAllocator;
	delete m_pRTVAllocator;
	delete m_pDSVAllocator;
}
