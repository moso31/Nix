#pragma once
#include "NXInstance.h"
#include "CommittedAllocator.h"
#include "PlacedAllocator.h"
#include "DescriptorAllocator.h"

// DX12 ∑÷≈‰∆˜
class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	void SetDX12Device(ID3D12Device* pDevice) { m_pDevice = pDevice; }
	void SetDX12CommandList(ID3D12CommandList* pCommandList) { m_pCommandList = pCommandList; }

	void Init();

	CommittedAllocator*		GetCBufferAllocator()		{ return m_pCBufferAllocator; }
	PlacedAllocator*		GetTextureAllocator()		{ return m_pTextureAllocator; }
	DescriptorAllocator*	GetDescriptorAllocator()	{ return m_pDescriptorAllocator; }

	void Release();

private:
	ID3D12Device* m_pDevice;
	ID3D12CommandList* m_pCommandList;

	CommittedAllocator*		m_pCBufferAllocator;
	PlacedAllocator*		m_pTextureAllocator;
	DescriptorAllocator*	m_pDescriptorAllocator;
};
