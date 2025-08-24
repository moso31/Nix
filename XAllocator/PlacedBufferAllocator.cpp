#include "PlacedBufferAllocator.h"

using namespace ccmem;

ccmem::PlacedBufferAllocator::PlacedBufferAllocator(const std::wstring& name, ID3D12Device* pDevice, uint32_t pageBlockByteSize, uint32_t pageFullByteSize) :
	m_pageFullByteSize(pageFullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(pageBlockByteSize, pageFullByteSize, name)
{
}

void ccmem::PlacedBufferAllocator::Alloc(D3D12_RESOURCE_DESC* desc, uint32_t byteSize, const std::function<void(const PlacedBufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::AddAllocTask(byteSize, desc, sizeof(D3D12_RESOURCE_DESC), [this, callback](const BuddyTaskResult& taskResult) {
		const auto& memData = taskResult.memData;
		PlacedBufferAllocTaskResult result;
		D3D12_RESOURCE_DESC* texDesc = reinterpret_cast<D3D12_RESOURCE_DESC*>(taskResult.pTaskContext);
		m_pDevice->CreatePlacedResource(m_allocatorMap[memData.pAllocator].pHeap, memData.byteOffset, texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&result.pResource));
		result.memData = memData;
		callback(result);
	});
}

void ccmem::PlacedBufferAllocator::Free(const XBuddyTaskMemData& memData)
{
	// 找到对应的内存块，然后标记为可以重新分配
	BuddyAllocator::AddFreeTask(memData.pAllocator, memData.byteOffset);
}

void ccmem::PlacedBufferAllocator::OnAllocatorAdded(BuddyAllocatorPage* pAllocator)
{
	D3D12_HEAP_DESC desc = {};
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Flags = D3D12_HEAP_FLAG_NONE;
	desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	desc.Properties.CreationNodeMask = 1; // Assuming single GPU node
	desc.Properties.VisibleNodeMask = 1; // Assuming single GPU node
	desc.SizeInBytes = m_pageFullByteSize;

	AllocatorData& newData = m_allocatorMap[pAllocator];

	m_pDevice->CreateHeap(&desc, IID_PPV_ARGS(&newData.pHeap));
	newData.pHeap->SetName(L"Heap");
}
