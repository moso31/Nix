#pragma once
#include "BaseDefs/DX12.h"
#include "NXInstance.h"
#include "CommittedAllocator.h"
#include "PlacedAllocator.h"
#include "DescriptorAllocator.h"
#include "RTVAllocator.h"
#include "DSVAllocator.h"
#include "NXShaderVisibleDescriptorHeap.h"

// DX12 ∑÷≈‰∆˜
class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void Init(ID3D12Device* pDevice);

	CommittedAllocator*		GetCBufferAllocator()		{ return m_pCBufferAllocator; }
	PlacedAllocator*		GetTextureAllocator()		{ return m_pTextureAllocator; }
	DescriptorAllocator*	GetDescriptorAllocator()	{ return m_pDescriptorAllocator; }
	RTVAllocator*			GetRTVAllocator()			{ return m_pRTVAllocator; }
	DSVAllocator*			GetDSVAllocator()			{ return m_pDSVAllocator; }

	NXShaderVisibleDescriptorHeap* GetShaderVisibleDescriptorHeap() { return m_pShaderVisibleDescriptorHeap; }

	void Release();

private:
	ID3D12CommandQueue*				m_pCommandQueue;
	ID3D12CommandAllocator*			m_pCommandAllocator;
	ID3D12CommandList*				m_pCommandList;

	CommittedAllocator*				m_pCBufferAllocator;
	PlacedAllocator*				m_pTextureAllocator;
	DescriptorAllocator*			m_pDescriptorAllocator;
	RTVAllocator*					m_pRTVAllocator;
	DSVAllocator*					m_pDSVAllocator;

	NXShaderVisibleDescriptorHeap*	m_pShaderVisibleDescriptorHeap;
};
