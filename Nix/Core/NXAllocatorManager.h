#pragma once
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "NXTextureLoader.h"

#define NXAllocator_CB		NXAllocatorManager::GetInstance()->GetCBAllocator()
#define NXAllocator_SB		NXAllocatorManager::GetInstance()->GetSBAllocator()
#define NXAllocator_Tex		NXAllocatorManager::GetInstance()->GetTextureAllocator()
#define NXAllocator_SRV		NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocator_RTV		NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocator_DSV		NXAllocatorManager::GetInstance()->GetDSVAllocator()
#define NXShVisDescHeap		NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorAllocator()
#define NXUploadSystem		NXAllocatorManager::GetInstance()->GetUploadSystem()
#define NXTexLoader			NXAllocatorManager::GetInstance()->GetTextureLoader()

using namespace ccmem;

class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init();

	CommittedBufferAllocator*			GetCBAllocator()			{ return m_pCBAllocator; }
	CommittedBufferAllocator*			GetSBAllocator()			{ return m_pSBAllocator; }
	PlacedBufferAllocator*				GetTextureAllocator()		{ return m_pTextureAllocator; }

	DescriptorAllocator<false>*			GetSRVAllocator()			{ return m_pSRVAllocator; }
	DescriptorAllocator<false>*			GetRTVAllocator()			{ return m_pRTVAllocator; }
	DescriptorAllocator<false>*			GetDSVAllocator()			{ return m_pDSVAllocator; }

	UploadSystem*		GetUploadSystem()	{ return m_pUpdateSystem; }
	NXTextureLoader*	GetTextureLoader()	{ return m_pTextureLoader; }

	DescriptorAllocator<true>*			GetShaderVisibleDescriptorAllocator()	{ return m_pShaderVisibleDescAllocator; }

	void Update();

	void Release();

private:
	CommittedBufferAllocator*			m_pCBAllocator;
	CommittedBufferAllocator*			m_pSBAllocator;
	PlacedBufferAllocator*				m_pTextureAllocator;

	DescriptorAllocator<false>*			m_pSRVAllocator;
	DescriptorAllocator<false>* 		m_pRTVAllocator;
	DescriptorAllocator<false>* 		m_pDSVAllocator;

	UploadSystem* m_pUpdateSystem;
	NXTextureLoader* m_pTextureLoader;

	// shader-visible descriptor allocator
	DescriptorAllocator<true>* 			m_pShaderVisibleDescAllocator;
};
