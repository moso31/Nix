#pragma once
#include "BaseDefs/DX12.h"

class NXVirtualTextureStreaming
{
public:
	NXVirtualTextureStreaming() {}
	~NXVirtualTextureStreaming() {}

	void Init();
	void Update();

private:
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
	ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_nFenceValue;
};
