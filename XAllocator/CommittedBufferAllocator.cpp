#include "CommittedBufferAllocator.h"

using namespace ccmem;

CommittedBufferAllocator::CommittedBufferAllocator(ID3D12Device* pDevice, bool cpuAccessable, uint32_t pageBlockByteSize, uint32_t pageFullByteSize) :
	m_cpuAccessable(cpuAccessable),
	m_pageFullByteSize(pageFullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(pageBlockByteSize, pageFullByteSize)
{
}

void CommittedBufferAllocator::Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::AddAllocTask(byteSize, nullptr, 0, [this, callback](const BuddyTaskResult& taskResult) {
		const auto& memData = taskResult.memData;
		CommittedBufferAllocTaskResult result;
		result.cpuAddress = m_cpuAccessable ? m_allocatorMap[memData.pAllocator].m_pResourceData + memData.byteOffset : nullptr; // 如果buffer CPU不可读，返回nullptr!
		result.gpuAddress = m_allocatorMap[memData.pAllocator].m_pResource->GetGPUVirtualAddress() + memData.byteOffset; 
		result.memData = memData;
		callback(result);
	});
}

void ccmem::CommittedBufferAllocator::Free(const XBuddyTaskMemData& memData)
{
	// 找到对应的内存块，然后标记为可以重新分配
	BuddyAllocator::AddFreeTask(memData.pAllocator, memData.byteOffset);
}

void ccmem::CommittedBufferAllocator::OnAllocatorAdded(BuddyAllocatorPage* pAllocator)
{
	D3D12_HEAP_PROPERTIES uploadHeapProps;
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProps.CreationNodeMask = 1;
	uploadHeapProps.VisibleNodeMask = 1;

	D3D12_HEAP_PROPERTIES defaultHeapProps;
	defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProps.CreationNodeMask = 1;
	defaultHeapProps.VisibleNodeMask = 1;

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

	AllocatorData& newData = m_allocatorMap[pAllocator];

	HRESULT hr = m_pDevice->CreateCommittedResource(
		m_cpuAccessable ? &uploadHeapProps : &defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // 资源描述
		m_cpuAccessable ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&newData.m_pResource)
	);

	if (m_cpuAccessable)
	{
		// 如果cpu可访问，构建映射地址
		newData.m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&newData.m_pResourceData));
	}

	// 设置调试用资源名称
	newData.m_pResource->SetName(L"CBuffer");
}

ID3D12Resource* ccmem::CommittedBufferAllocator::GetD3DResource(BuddyAllocatorPage* pAllocator)
{
	return m_allocatorMap[pAllocator].m_pResource;
}
