#pragma once
#include "DXIncludes.h"
#include "XAllocator.h"

// why 1000000? see https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define DESCRIPTOR_NUM_PER_HEAP_MAXLIMIT 1000000

enum DescriptorType
{
	DescriptorType_CBV,
	DescriptorType_SRV,
	DescriptorType_UAV,
};

struct DescriptorPage
{
	ID3D12DescriptorHeap* data = nullptr;
	DescriptorType type;
};

#define DescriptorAllocatorBase XAllocator<DescriptorPage>

class DescriptorAllocator : public DescriptorAllocatorBase
{
public:
	DescriptorAllocator(ID3D12Device* pDevice);
	DescriptorAllocator(ID3D12Device* pDevice, UINT pageNumLimit, UINT pageSizeLimit);

	// �ڶ�����һ�δ�СΪ allocSize �Ŀռ䣬������������
	bool Alloc(DescriptorType type, UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandles);
	bool Alloc(DescriptorType type, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle);

	// �Ƴ� pageIdx ҳ��ģ��� start ��ʼ����Ϊ size ���ڴ��
	void Remove(UINT pageIdx, UINT start, UINT size);

	void CreateNewPage(DescriptorAllocatorBase::Page& newPage) override;

private:
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;
};
