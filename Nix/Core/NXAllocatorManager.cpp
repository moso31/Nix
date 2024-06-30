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

	// Shader visible heap Ԥ��10����̬��������Ŀǰ��ô����ԭ����ʱֻ��һ����imgui
	// ��imgui DX12�У�����ʹ�����������й̶�λ�õ���������Ϊ�����������������
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
