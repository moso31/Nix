#pragma once
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "NXTextureLoader.h"
#include "NXNullDescriptor.h"

class NXVirtualTextureStreaming;

// ��Դ
#define NXAllocator_CB		NXAllocatorManager::GetInstance()->GetCBAllocator()			// cb
#define NXAllocator_SB		NXAllocatorManager::GetInstance()->GetSBAllocator()			// vb, ib, instance-data
#define NXAllocator_RB		NXAllocatorManager::GetInstance()->GetRBAllocator()			// readback buffer
#define NXAllocator_RWB		NXAllocatorManager::GetInstance()->GetRWBAllocator()		// rw buffer
#define NXAllocator_Tex		NXAllocatorManager::GetInstance()->GetTextureAllocator()	// texture��������Ŀǰ���ļ����߶�ͼ��cpu���������⼸�֡���������RT��RT������

// ������
#define NXAllocator_SRV		NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocator_RTV		NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocator_DSV		NXAllocatorManager::GetInstance()->GetDSVAllocator()
#define NXAllocator_NULL 	NXAllocatorManager::GetInstance()->GetNullDescriptorAllocator()
#define NXShVisDescHeap		NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorAllocator()

// �ϴ�ϵͳ
#define NXUploadSys			NXAllocatorManager::GetInstance()->GetNXUploadSystem()
#define NXReadbackSys		NXAllocatorManager::GetInstance()->GetNXReadbackSystem()

// ���������
#define NXTexLoader			NXAllocatorManager::GetInstance()->GetTextureLoader()

// VT Streaming
#define NXVTStreaming		NXAllocatorManager::GetInstance()->GetVirtualTextureStreaming()

using namespace ccmem;

class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	NXAllocatorManager();
	virtual ~NXAllocatorManager();

	void Init();

	CommittedBufferAllocator*		GetCBAllocator()				{ return m_pCBAllocator.get(); }
	CommittedBufferAllocator*		GetSBAllocator()				{ return m_pSBAllocator.get(); }
	CommittedBufferAllocator*		GetRBAllocator()				{ return m_pRBAllocator.get(); }
	PlacedBufferAllocator*			GetRWBAllocator()				{ return m_pRWBAllocator.get(); }
	PlacedBufferAllocator*			GetTextureAllocator()			{ return m_pTextureAllocator.get(); }

	DescriptorAllocator<false>*		GetSRVAllocator()				{ return m_pSRVAllocator.get(); }
	DescriptorAllocator<false>*		GetRTVAllocator()				{ return m_pRTVAllocator.get(); }
	DescriptorAllocator<false>*		GetDSVAllocator()				{ return m_pDSVAllocator.get(); }
	NXNullDescriptor*				GetNullDescriptorAllocator()	{ return m_pNullDescriptorAllocator.get(); }

	NXUploadSystem*					GetNXUploadSystem()				{ return m_pUpdateSystem.get(); }
	NXReadbackSystem*				GetNXReadbackSystem()			{ return m_pReadbackSystem.get(); }
	NXTextureLoader*				GetTextureLoader()				{ return m_pTextureLoader.get(); }
	NXVirtualTextureStreaming*		GetVirtualTextureStreaming()	{ return m_pVTStreaming.get(); }

	DescriptorAllocator<true>*		GetShaderVisibleDescriptorAllocator()	{ return m_pShaderVisibleDescAllocator.get(); }

	void Release();

private:
	std::unique_ptr<CommittedBufferAllocator>	m_pCBAllocator;	
	std::unique_ptr<CommittedBufferAllocator>	m_pSBAllocator;
	std::unique_ptr<CommittedBufferAllocator>	m_pRBAllocator;
	std::unique_ptr<PlacedBufferAllocator>		m_pRWBAllocator;
	std::unique_ptr<PlacedBufferAllocator>		m_pTextureAllocator;

	std::unique_ptr<DescriptorAllocator<false>>	m_pSRVAllocator;
	std::unique_ptr<DescriptorAllocator<false>>	m_pRTVAllocator;
	std::unique_ptr<DescriptorAllocator<false>>	m_pDSVAllocator;
	std::unique_ptr<NXNullDescriptor>			m_pNullDescriptorAllocator;

	std::unique_ptr<NXUploadSystem>				m_pUpdateSystem;
	std::unique_ptr<NXReadbackSystem>			m_pReadbackSystem;
	std::unique_ptr<NXTextureLoader>			m_pTextureLoader;
	std::unique_ptr<NXVirtualTextureStreaming>	m_pVTStreaming;

	// shader-visible descriptor allocator
	std::unique_ptr<DescriptorAllocator<true>> 	m_pShaderVisibleDescAllocator;

	// �������ж�ʹ�ö����߳����У�ʹ��һ���߳�������������
	std::vector<std::thread> m_threads;
	std::atomic<bool> m_bRunning;
};
