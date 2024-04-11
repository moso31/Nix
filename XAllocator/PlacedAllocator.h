#pragma once
#include "XAllocator.h"

#define PlacedAllocatorBase XAllocator<ID3D12Heap*>

class PlacedAllocator : public PlacedAllocatorBase
{
public:
	// 2024.4.9 DX12 要求 blockByteSize 对齐 65536 (即 64KB)
	PlacedAllocator(ID3D12Device* pDevice, UINT blockByteSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) : 
		PlacedAllocatorBase(1000000, 10), m_pDevice(pDevice), m_blockByteSize(blockByteSize) {}
	~PlacedAllocator() {}

	bool Alloc(const D3D12_RESOURCE_DESC& resourceDesc, ID3D12Resource** pOutResource);

	void CreateNewPage(PlacedAllocatorBase::Page& newPage) override;

private:
	ID3D12Device* m_pDevice;

	UINT m_blockByteSize; // 每个page里的每个block所占的字节大小（该值必须是256的整数倍）
};
