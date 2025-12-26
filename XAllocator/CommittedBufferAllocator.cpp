#include "CommittedBufferAllocator.h"

using namespace ccmem;

CommittedBufferAllocator::CommittedBufferAllocator(const std::wstring& name, ID3D12Device* pDevice, bool cpuAccessable, bool isReadBack, uint32_t pageBlockByteSize, uint32_t pageFullByteSize) :
	m_cpuAccessable(cpuAccessable),
	m_isReadBack(isReadBack),
	m_pageFullByteSize(pageFullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(pageBlockByteSize, pageFullByteSize, name)
{
	if (m_isReadBack && pageBlockByteSize < 512u)
	{
		assert(false);
	}
}

void CommittedBufferAllocator::Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::AddAllocTask(byteSize, nullptr, 0, [this, callback](const BuddyTaskResult& taskResult) {
		const auto& memData = taskResult.memData;
		CommittedBufferAllocTaskResult result;

		if (m_cpuAccessable || m_isReadBack)
			result.cpuAddress = m_allocatorMap[memData.pAllocator].m_pResourceData + memData.byteOffset;
		else
			result.cpuAddress = nullptr; 

		if (m_isReadBack)
			result.gpuResource = nullptr;
		else
			result.gpuResource = m_allocatorMap[memData.pAllocator].m_pResource;

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
	D3D12_HEAP_PROPERTIES props{};
	props.Type = m_isReadBack ? D3D12_HEAP_TYPE_READBACK : 
		m_cpuAccessable ? D3D12_HEAP_TYPE_UPLOAD: D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC cbDesc;
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Alignment = 0; 
	cbDesc.Width = m_pageFullByteSize;
	cbDesc.Height = 1; 
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1; 
	cbDesc.Format = DXGI_FORMAT_UNKNOWN; 
	cbDesc.SampleDesc.Count = 1; 
	cbDesc.SampleDesc.Quality = 0; 
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; 
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_STATES initState = m_isReadBack ? D3D12_RESOURCE_STATE_COPY_DEST :
		m_cpuAccessable ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

	AllocatorData& newData = m_allocatorMap[pAllocator];
	HRESULT hr = m_pDevice->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // 资源描述
		initState,
		nullptr,
		IID_PPV_ARGS(&newData.m_pResource)
	);

	if (m_cpuAccessable || m_isReadBack)
	{
		// 如果cpu可访问，构建映射地址
		newData.m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&newData.m_pResourceData));
	}

	// 设置调试用资源名称
	std::wstring bufferName = GetName() + std::to_wstring(pAllocator->GetPageID());
	newData.m_pResource->SetName(bufferName.c_str());
}

ID3D12Resource* ccmem::CommittedBufferAllocator::GetD3DResource(BuddyAllocatorPage* pAllocator)
{
	return m_allocatorMap[pAllocator].m_pResource;
}
