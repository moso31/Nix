#pragma once
#include "XAllocator.h"

template <typename T>
struct CommittedResourceData
{
	UINT DataByteSize() { return sizeof(T); }
	const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc() { return { GPUVirtualAddr, AlignedDataByteSize() }; }

	T data;
	UINT pageIndex; // ��¼�������� XAllocator ��ҳ������
	UINT pageByteOffset; // ��¼�������� XAllocator ��ҳ����ֽ�ƫ����
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // ��¼�����ݵ� GPU �����ַ

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

	// ����Դ���з���һ���ڴ档
	// byteSize��
	//		Ҫ��������ݵ��ֽڴ�С��ע�ⲻ��ʵ�ʷ�����ֽڴ�С��
	// oGPUVirtualAddr��
	//		��������ݶ�Ӧ��GPU��ַ��
	//		DX12��ȷ������ʱ����Ҫ�ṩGPU��ַ��
	// oPageIdx��
	//		���η�������ݣ���Դ��ҳ��ı�š�
	// oPageByteOffset��
	//		���η�������� ����Դ��ҳ���е�ʵ����ʼ�ֽڡ�
	bool Alloc(UINT byteSize, ResourceType resourceType, D3D12_GPU_VIRTUAL_ADDRESS& oGPUVirtualAddr, UINT& oPageIdx, UINT& oPageByteOffset);

	template <typename T>
	bool Alloc(ResourceType type, CommittedResourceData<T>& info)
	{
		return Alloc(info.DataByteSize(), type, info.GPUVirtualAddr, info.pageIndex, info.pageByteOffset);
	}
	 
	// ������Դ���е��ڴ档
	// NOTE���� ResourceType_Upload ���͵� Page ����ʹ�ô˷�����
	void UpdateData(void* data, UINT dataSize, UINT pageIdx, UINT pageByteOffset);

	// ������Դ���е��ڴ档
	// NOTE���� ResourceType_Upload ���͵� Page ����ʹ�ô˷�����
	template <typename T>
	void UpdateData(CommittedResourceData<T>& info)
	{
		return UpdateData(info.data, info.DataByteSize(), info.pageIndex, info.pageByteOffset);
	}

	// ������Դ���е��ڴ档
	// NOTE���� ResourceType_Default ���͵� Page ����ʹ�ô˷�����
	void UpdateData(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, ID3D12Resource* pUploadResource, UINT dataSize, UINT pageIdx, UINT pageByteOffset);

	// ������Դ״̬
	void SetResourceState(ID3D12GraphicsCommandList* ID3D12GraphicsCommandList, UINT pageIdx, const D3D12_RESOURCE_STATES& state);

	void CreateNewPage(CommittedAllocatorBase::Page& newPage) override;

private:
	ID3D12Device* m_pDevice;

	UINT m_blockByteSize; // ÿ��page���ÿ��block��ռ���ֽڴ�С����ֵ������256����������
};