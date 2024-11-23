#include "CommittedBufferAllocator.h"

using namespace ccmem;

CommittedBufferAllocator::CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t pageBlockByteSize, uint32_t pageFullByteSize) :
	m_pageFullByteSize(pageFullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(pageBlockByteSize, pageFullByteSize)
{
}

void CommittedBufferAllocator::Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::AddAllocTask(byteSize, nullptr, 0, [this, callback](const BuddyTaskResult& taskResult) {
		CommittedBufferAllocTaskResult result;
		result.cpuAddress = m_allocatorPageData[taskResult.pAllocator].m_pResourceData + taskResult.byteOffset;
		result.gpuAddress = m_allocatorPageData[taskResult.pAllocator].m_pResource->GetGPUVirtualAddress() + taskResult.byteOffset;

		m_freeMap[result.cpuAddress].byteOffset = taskResult.byteOffset;
		m_freeMap[result.cpuAddress].pAllocator = taskResult.pAllocator;
		callback(result);
	});
}

void ccmem::CommittedBufferAllocator::Free(uint8_t* pMem)
{
	// 找到对应的内存块，然后标记为可以重新分配
	if (m_freeMap.find(pMem) != m_freeMap.end())
	{
		auto& data = m_freeMap[pMem];
		BuddyAllocator::AddFreeTask(data.pAllocator, data.byteOffset);
		m_freeMap.erase(pMem);
	}
}

void ccmem::CommittedBufferAllocator::OnAllocatorAdded(BuddyAllocatorPage* pAllocator)
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
	cbDesc.Width = m_pageFullByteSize;
	cbDesc.Height = 1; 
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1; 
	cbDesc.Format = DXGI_FORMAT_UNKNOWN; 
	cbDesc.SampleDesc.Count = 1; 
	cbDesc.SampleDesc.Quality = 0; 
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; 
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	AllocatorData& newData = m_allocatorPageData[pAllocator];

	HRESULT hr = m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // 资源描述
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&newData.m_pResource)
	);

	// 构建映射地址
	newData.m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&newData.m_pResourceData));

	// 设置调试用资源名称
	newData.m_pResource->SetName(L"CBuffer");
}
