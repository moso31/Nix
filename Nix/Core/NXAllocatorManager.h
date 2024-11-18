#pragma once
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"

#define NXAllocator_CB		NXAllocatorManager::GetInstance()->GetCBAllocator()
#define NXAllocator_SB		NXAllocatorManager::GetInstance()->GetSBAllocator()
#define NXAllocator_Tex		NXAllocatorManager::GetInstance()->GetTextureAllocator()
#define NXAllocator_SRV		NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocator_RTV		NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocator_DSV		NXAllocatorManager::GetInstance()->GetDSVAllocator()
#define NXShVisDescHeap		NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorAllocator()
#define NXUploadSystem		NXAllocatorManager::GetInstance()->GetUploadSystem()

using namespace ccmem;

class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init();

	CommittedBufferAllocator*			GetCBAllocator()			{ return m_pCBAllocator; }
	CommittedBufferAllocator*			GetSBAllocator()			{ return m_pSBAllocator; }
	PlacedBufferAllocator*				GetTextureAllocator()		{ return m_pTextureAllocator; }
	UploadSystem*						GetUploadSystem()			{ return m_pUpdateSystem; }

	DescriptorAllocator<false>*			GetSRVAllocator()			{ return m_pSRVAllocator; }
	DescriptorAllocator<false>*			GetRTVAllocator()			{ return m_pRTVAllocator; }
	DescriptorAllocator<false>*			GetDSVAllocator()			{ return m_pDSVAllocator; }

	DescriptorAllocator<true>*			GetShaderVisibleDescriptorAllocator()	{ return m_pShaderVisibleDescAllocator; }

	void Update();

	void Release();

private:
	CommittedBufferAllocator*			m_pCBAllocator;
	CommittedBufferAllocator*			m_pSBAllocator;
	PlacedBufferAllocator*				m_pTextureAllocator;
	UploadSystem*						m_pUpdateSystem;

	DescriptorAllocator<false>*			m_pSRVAllocator;
	DescriptorAllocator<false>* 		m_pRTVAllocator;
	DescriptorAllocator<false>* 		m_pDSVAllocator;

	// shader-visible descriptor allocator
	DescriptorAllocator<true>* 			m_pShaderVisibleDescAllocator;
};
