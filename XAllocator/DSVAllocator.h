#pragma once
#include "DXIncludes.h"
#include "XAllocator.h"

#define DESCRIPTOR_NUM_PER_HEAP_DSV 1000

#define DSVAllocatorBase XAllocator<ID3D12DescriptorHeap*>

class DSVAllocator : public DSVAllocatorBase
{
public:
	DSVAllocator(ID3D12Device* pDevice);

	// �ڶ�����һ�δ�СΪ allocSize �Ŀռ䣬������������
	bool Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& oHandle);
	bool Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle);

	// �Ƴ� pageIdx ҳ��ģ��� start ��ʼ����Ϊ size ���ڴ��
	void Remove(UINT pageIdx, UINT start, UINT size);

	void CreateNewPage(DSVAllocatorBase::Page& newPage) override;

private:
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;
};
