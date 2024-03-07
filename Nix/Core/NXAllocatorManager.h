#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"

#define NXCBufferAllocator			NXAllocatorManager::GetInstance()->GetCBufferAllocator()
#define NXTextureAllocator			NXAllocatorManager::GetInstance()->GetTextureAllocator()
#define NXDescriptorAllocator		NXAllocatorManager::GetInstance()->GetDescriptorAllocator()
#define NXGPUHandleHeap				NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap()
#define NXCmdList					NXAllocatorManager::GetInstance()->GetCommandList()

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

	ID3D12CommandList* GetCommandList() { return m_pCommandList; }
	ID3D12CommandQueue* GetCommandQueue() { return m_pCommandQueue; }
	ID3D12CommandAllocator* GetCommandAllocator() { return m_pCommandAllocator; }

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
