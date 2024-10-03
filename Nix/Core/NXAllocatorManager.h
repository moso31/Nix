#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"

//#define NXCBufferAllocator			NXAllocatorManager::GetInstance()->GetCBufferAllocator()
//#define NXTextureAllocator			NXAllocatorManager::GetInstance()->GetTextureAllocator()
//#define NXDescriptorAllocator		NXAllocatorManager::GetInstance()->GetDescriptorAllocator()
//#define NXGPUHandleHeap				NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap()

#define NXAllocMng_CB			 NXAllocatorManager::GetInstance()->GetCBAllocator()
#define NXAllocMng_SRV			 NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocMng_RTV			 NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocMng_DSV			 NXAllocatorManager::GetInstance()->GetDSVAllocator()

using namespace ccmem;

// DX12 ∑÷≈‰∆˜
class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init();

	CommittedBufferAllocator*			GetCBAllocator()	{ return m_pCBAllocator; }
	CommittedBufferAllocator*			GetSBAllocator()	{ return m_pSBAllocator; }
	ShaderVisibleDescriptorAllocator*	GetSRVAllocator()	{ return m_pSRVAllocator; }
	NonVisibleDescriptorAllocator*		GetRTVAllocator()	{ return m_pRTVAllocator; }
	NonVisibleDescriptorAllocator*		GetDSVAllocator()	{ return m_pDSVAllocator; }

	//NonVisibleDescriptorAllocator*		GetDescriptorAllocator()				{ return m_p???Allocator; }
	//CommittedBufferAllocator* GetCBufferAllocator() { return m_pConstantBufferAllocator; }
	//PlacedAllocator*		GetTextureAllocator()			{ return m_pTextureAllocator; }
	//RTVAllocator*			GetRTVAllocator()			{ return m_pRTVAllocator; }
	//DSVAllocator*			GetDSVAllocator()			{ return m_pDSVAllocator; }

	NXShaderVisibleDescriptorHeap* GetShaderVisibleDescriptorHeap() { return m_pShaderVisibleDescriptorHeap; }

	void Release();

private:
	CommittedBufferAllocator*			m_pCBAllocator;
	CommittedBufferAllocator*			m_pSBAllocator;
	ShaderVisibleDescriptorAllocator*	m_pSRVAllocator;
	NonVisibleDescriptorAllocator* 		m_pRTVAllocator;
	NonVisibleDescriptorAllocator* 		m_pDSVAllocator;
	ShaderVisibleDescriptorAllocator*	m_pShaderVisibleDescriptorAllocator;

	//PlacedAllocator*				m_pTextureAllocator;
	//DescriptorAllocator*			m_pDescriptorAllocator;
	//RTVAllocator*					m_pRTVAllocator;
	//DSVAllocator*					m_pDSVAllocator;

	NXShaderVisibleDescriptorHeap*	m_pShaderVisibleDescriptorHeap;
};
