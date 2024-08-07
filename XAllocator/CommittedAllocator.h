#pragma once
#include "XAllocator.h"

enum ResourceType
{
	ResourceType_Default,
	ResourceType_Upload,
};

struct CommittedResourcePage
{
	ID3D12Resource* pResource;
	UINT8* pResourceMap; // ResourceType_Upload use only.
	ResourceType type;
	D3D12_RESOURCE_STATES resourceState;
};

#define CommittedAllocatorBase XAllocator<CommittedResourcePage>

class CommittedAllocator : public CommittedAllocatorBase
{
public:
	CommittedAllocator(ID3D12Device* pDevice, UINT blockByteSize = 256);
	~CommittedAllocator() {}

	ID3D12Device* GetD3DDevice() { return m_pDevice; }

	// 在资源池中分配一段内存。
	// byteSize：
	//		要分配的数据的字节大小（实际分配的字节大小会做blockByteSize对齐，可能更大一些）
	// oGPUVirtualAddr：
	//		分配的数据对应的GPU地址。
	//		DX12明确根参数时，需要提供GPU地址。
	// oPageIdx：
	//		本次分配的数据，资源池页面的编号。
	// oPageByteOffset：
	//		本次分配的数据 在资源池页面中的实际起始字节。
	bool Alloc(UINT byteSize, ResourceType resourceType, D3D12_GPU_VIRTUAL_ADDRESS& oGPUVirtualAddr, UINT& oPageIdx, UINT& oPageByteOffset);

	// 释放资源池中的内存。
	// pageIdx：
	//		要释放的数据所在的资源池页面编号。
	// pageByteOffset：
	//		要释放的数据在资源池页面中的实际起始字节。
	// byteSize：
	//		要释放的数据的字节大小。
	void Remove(UINT pageIdx, UINT pageByteOffset, UINT byteSize);
	 
	// 更新资源池中的内存。
	// NOTE：仅 ResourceType_Upload 类型的 Page 可以使用此方法！
	void UpdateData(const void* data, UINT dataSize, UINT pageIdx, UINT pageByteOffset);

	// 更新资源池中的内存。
	// NOTE：仅 ResourceType_Default 类型的 Page 可以使用此方法！
	void UpdateData(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, ID3D12Resource* pUploadResource, UINT srcDataOffset, UINT srcDataSize, UINT pageIdx, UINT pageByteOffset);

	// 设置资源状态
	void SetResourceState(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, UINT pageIdx, const D3D12_RESOURCE_STATES& state);

	// 新分配一个Page
	void CreateNewPage(CommittedAllocatorBase::Page& newPage) override;

	// 释放资源池中的所有资源
	void Clear() override;

private:
	ID3D12Device* m_pDevice;

	UINT m_blockByteSize; // 每个page里的每个block所占的字节大小（该值必须是256的整数倍）
};
