#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"

#define NXCBufferAllocator			NXAllocatorManager::GetInstance()->GetCBufferAllocator()
#define NXTextureAllocator			NXAllocatorManager::GetInstance()->GetTextureAllocator()
#define NXDescriptorAllocator		NXAllocatorManager::GetInstance()->GetDescriptorAllocator()
#define NXGPUHandleHeap				NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap()

// DX12 ������
class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init();

	CommittedAllocator*		GetCBufferAllocator()		{ return m_pCBufferAllocator; }
	PlacedAllocator*		GetTextureAllocator()		{ return m_pTextureAllocator; }
	DescriptorAllocator*	GetDescriptorAllocator()	{ return m_pDescriptorAllocator; }
	RTVAllocator*			GetRTVAllocator()			{ return m_pRTVAllocator; }
	DSVAllocator*			GetDSVAllocator()			{ return m_pDSVAllocator; }

	NXShaderVisibleDescriptorHeap* GetShaderVisibleDescriptorHeap() { return m_pShaderVisibleDescriptorHeap; }

	void Release();

private:
	CommittedAllocator*				m_pCBufferAllocator;
	PlacedAllocator*				m_pTextureAllocator;
	DescriptorAllocator*			m_pDescriptorAllocator;
	RTVAllocator*					m_pRTVAllocator;
	DSVAllocator*					m_pDSVAllocator;

	NXShaderVisibleDescriptorHeap*	m_pShaderVisibleDescriptorHeap;
};
