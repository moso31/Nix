#include "ConstantBufferAllocator.h"

using namespace ccmem;

CommittedBufferAllocator::CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t blockByteSize, uint32_t fullByteSize) :
	m_fullByteSize(fullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(blockByteSize, fullByteSize)
{
}

void CommittedBufferAllocator::Alloc(uint32_t byteSize, const std::function<void(const BufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::Alloc(byteSize, [this, callback](const BuddyTaskResult& taskResult) {
		BufferAllocTaskResult result;
		result.cpuAddress = taskResult.pMemory;
		result.gpuAddress = m_pResource->GetGPUVirtualAddress() + (taskResult.pMemory - m_pResourceData);
		callback(result);
	});
}

void ccmem::CommittedBufferAllocator::Free(uint8_t* pMem)
{
	BuddyAllocator::Free(pMem);
}

void ccmem::CommittedBufferAllocator::OnAllocatorAdded()
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC cbDesc;
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	cbDesc.Width = m_fullByteSize; // 总大小，单位为字节
	cbDesc.Height = 1; // 对于缓冲区，高度必须为1
	cbDesc.DepthOrArraySize = 1; // 对于缓冲区，深度必须为1
	cbDesc.MipLevels = 1; // 缓冲区的MIP等级数，应为1
	cbDesc.Format = DXGI_FORMAT_UNKNOWN; // 缓冲区不使用DXGI格式，所以设置为UNKNOWN
	cbDesc.SampleDesc.Count = 1; // 多重采样的数量，对于缓冲区，这应该是1
	cbDesc.SampleDesc.Quality = 0; // 多重采样的质量，通常为0
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 缓冲区的布局，通常为行主序
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 资源的标志，根据需要进行设置

	HRESULT hr = m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // 资源描述
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr,
		IID_PPV_ARGS(&m_pResource)
	);

	// 构建映射地址
	m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));

	// 设置调试用资源名称
	m_pResource->SetName(L"CBuffer");
}
