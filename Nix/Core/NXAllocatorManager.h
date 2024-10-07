#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"

//#define NXCBufferAllocator			NXAllocatorManager::GetInstance()->GetCBufferAllocator()
//#define NXTextureAllocator			NXAllocatorManager::GetInstance()->GetTextureAllocator()
//#define NXDescriptorAllocator		NXAllocatorManager::GetInstance()->GetDescriptorAllocator()
//#define NXGPUHandleHeap				NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap()

#define NXAllocator_CB		NXAllocatorManager::GetInstance()->GetCBAllocator()
#define NXAllocator_SB		NXAllocatorManager::GetInstance()->GetSBAllocator()
#define NXAllocator_Tex		NXAllocatorManager::GetInstance()->GetTextureAllocator()
#define NXAllocator_SRV		NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocator_RTV		NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocator_DSV		NXAllocatorManager::GetInstance()->GetDSVAllocator()
#define NXUploadSystem		NXAllocatorManager::GetInstance()->GetUploadSystem()

using namespace ccmem;

// DX12 ������
class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init();

	CommittedBufferAllocator*			GetCBAllocator()		{ return m_pCBAllocator; }
	CommittedBufferAllocator*			GetSBAllocator()		{ return m_pSBAllocator; }
	PlacedBufferAllocator*				GetTextureAllocator()	{ return m_pTextureAllocator; }
	UploadSystem*						GetUploadSystem()		{ return m_pUpdateSystem; }

	ShaderVisibleDescriptorAllocator*	GetSRVAllocator()		{ return m_pSRVAllocator; }
	NonVisibleDescriptorAllocator*		GetRTVAllocator()		{ return m_pRTVAllocator; }
	NonVisibleDescriptorAllocator*		GetDSVAllocator()		{ return m_pDSVAllocator; }

	void Release();

private:
	CommittedBufferAllocator*			m_pCBAllocator;
	CommittedBufferAllocator*			m_pSBAllocator;
	PlacedBufferAllocator*				m_pTextureAllocator;
	UploadSystem*						m_pUpdateSystem;

	ShaderVisibleDescriptorAllocator*	m_pSRVAllocator;
	NonVisibleDescriptorAllocator* 		m_pRTVAllocator;
	NonVisibleDescriptorAllocator* 		m_pDSVAllocator;
};
