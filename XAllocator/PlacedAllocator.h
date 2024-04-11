#pragma once
#include "XAllocator.h"

#define PlacedAllocatorBase XAllocator<ID3D12Heap*>

class PlacedAllocator : public PlacedAllocatorBase
{
public:
	// 2024.4.9 DX12 Ҫ�� blockByteSize ���� 65536 (�� 64KB)
	PlacedAllocator(ID3D12Device* pDevice, UINT blockByteSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) : 
		PlacedAllocatorBase(1000000, 10), m_pDevice(pDevice), m_blockByteSize(blockByteSize) {}
	~PlacedAllocator() {}

	bool Alloc(const D3D12_RESOURCE_DESC& resourceDesc, ID3D12Resource** pOutResource);

	void CreateNewPage(PlacedAllocatorBase::Page& newPage) override;

private:
	ID3D12Device* m_pDevice;

	UINT m_blockByteSize; // ÿ��page���ÿ��block��ռ���ֽڴ�С����ֵ������256����������
};
