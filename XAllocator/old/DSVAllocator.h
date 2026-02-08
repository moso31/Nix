#pragma once
#include "DXIncludes.h"
#include "XAllocator.h"

#define DESCRIPTOR_NUM_PER_HEAP_DSV 1000

#define DSVAllocatorBase XAllocator<ID3D12DescriptorHeap*>

class DSVAllocator : public DSVAllocatorBase
{
public:
	DSVAllocator(ID3D12Device* pDevice);

	// 在堆里找一段大小为 allocSize 的空间，并分配描述符
	bool Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& oHandle);
	bool Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle);

	// 移除 pageIdx 页面的，从 start 开始长度为 size 的内存块
	void Remove(UINT pageIdx, UINT start, UINT size);

	void CreateNewPage(DSVAllocatorBase::Page& newPage) override;

	void Clear() override;

private:
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;
};
