#pragma once
#include "XAllocator.h"

template <typename T>
struct CommittedResourceData
{
	UINT DataByteSize() { return sizeof(T); }
	const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc() { return { GPUVirtualAddr, AlignedDataByteSize() }; }

	T data;
	UINT pageIndex; // 记录该数据在 XAllocator 的页面索引
	UINT pageByteOffset; // 记录该数据在 XAllocator 的页面的字节偏移量
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // 记录该数据的 GPU 虚拟地址

private:
	UINT AlignedDataByteSize() { return (sizeof(T) + 255) & ~255; }
};

enum ResourceType
{
	ResourceType_Default,
	ResourceType_Upload,
};

struct CommittedResourcePage
{
	ID3D12Resource* pResource;
	ResourceType type;
	D3D12_RESOURCE_STATES resourceState;
};

#define CommittedAllocatorBase XAllocator<CommittedResourcePage>

class CommittedAllocator : public CommittedAllocatorBase
{
public:
	CommittedAllocator(ID3D12Device* pDevice, UINT blockByteSize = 256) : 
		CommittedAllocatorBase(1000000, 100), m_pDevice(pDevice), m_blockByteSize(blockByteSize) {}
	~CommittedAllocator() {}

	// 在资源池中分配一段内存。
	// byteSize：
	//		要分配的数据的字节大小（注意不是实际分配的字节大小）
	// oGPUVirtualAddr：
	//		分配的数据对应的GPU地址。
	//		DX12明确根参数时，需要提供GPU地址。
	// oPageIdx：
	//		本次分配的数据，资源池页面的编号。
	// oPageByteOffset：
	//		本次分配的数据 在资源池页面中的实际起始字节。
	bool Alloc(UINT byteSize, ResourceType resourceType, D3D12_GPU_VIRTUAL_ADDRESS& oGPUVirtualAddr, UINT& oPageIdx, UINT& oPageByteOffset);

	template <typename T>
	bool Alloc(ResourceType type, CommittedResourceData<T>& info)
	{
		return Alloc(info.DataByteSize(), type, info.GPUVirtualAddr, info.pageIndex, info.pageByteOffset);
	}
	 
	// 更新资源池中的内存。
	// NOTE：仅 ResourceType_Upload 类型的 Page 可以使用此方法！
	void UpdateData(void* data, UINT dataSize, UINT pageIdx, UINT pageByteOffset);

	// 更新资源池中的内存。
	// NOTE：仅 ResourceType_Upload 类型的 Page 可以使用此方法！
	template <typename T>
	void UpdateData(CommittedResourceData<T>& info)
	{
		return UpdateData(info.data, info.DataByteSize(), info.pageIndex, info.pageByteOffset);
	}

	// 更新资源池中的内存。
	// NOTE：仅 ResourceType_Default 类型的 Page 可以使用此方法！
	void UpdateData(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, ID3D12Resource* pUploadResource, UINT dataSize, UINT pageIdx, UINT pageByteOffset);

	// 设置资源状态
	void SetResourceState(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, UINT pageIdx, const D3D12_RESOURCE_STATES& state);

	void CreateNewPage(CommittedAllocatorBase::Page& newPage) override;

private:
	ID3D12Device* m_pDevice;

	UINT m_blockByteSize; // 每个page里的每个block所占的字节大小（该值必须是256的整数倍）
};
