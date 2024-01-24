#include "CommittedAllocator.h"

bool CommittedAllocator::Alloc(UINT byteSize, ResourceType resourceType, D3D12_GPU_VIRTUAL_ADDRESS& oGPUVirtualAddr, UINT& oPageIdx, UINT& oPageByteOffset)
{
	size_t blockByteMask = m_blockByteSize - 1;
	UINT dataByteSize = (UINT)((byteSize + blockByteMask) & ~blockByteMask);
	UINT blockSize = dataByteSize / m_blockByteSize; // 这次alloc需要使用多少个Block

	auto predicate = [resourceType](Page& page) {
		return page.data.type == resourceType;
	};

	auto onCreate = [resourceType](Page& page) {
		page.data.type = resourceType;
	};

	UINT oFirstIdx;
	if (CommittedAllocatorBase::Alloc(blockSize, oPageIdx, oFirstIdx, predicate, onCreate))
	{
		auto& pResource = m_pages[oPageIdx].data.pResource;
		oPageByteOffset = m_blockByteSize * oFirstIdx;
		oGPUVirtualAddr = pResource->GetGPUVirtualAddress() + oPageByteOffset;
		return true;
	}

	return false;
}

void CommittedAllocator::UpdateData(void* data, UINT dataSize, UINT pageIdx, UINT pageByteOffset)
{
	auto& pResource = m_pages[pageIdx].data.pResource;

	// 只 Map 要修改的那一段即可
	D3D12_RANGE mapRange;
	mapRange.Begin = pageByteOffset;
	mapRange.End = pageByteOffset + dataSize;

	UINT8* pSrc;
	HRESULT hr = pResource->Map(0, &mapRange, reinterpret_cast<void**>(&pSrc));

	UINT8* pDest = pSrc + pageByteOffset;
	memcpy(pDest, data, dataSize);
}

void CommittedAllocator::UpdateData(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pUploadResource, UINT dataSize, UINT pageIdx, UINT pageByteOffset)
{
	auto& pResource = m_pages[pageIdx].data.pResource;
	pCmdList->CopyBufferRegion(pResource, pageByteOffset, pUploadResource, pageByteOffset, dataSize);
}

void CommittedAllocator::SetResourceState(ID3D12GraphicsCommandList* pCmdList, UINT pageIdx, const D3D12_RESOURCE_STATES& state)
{
	auto& resourceState = m_pages[pageIdx].data.resourceState;

	if (resourceState != state)
	{
		auto& pResource = m_pages[pageIdx].data.pResource;

		D3D12_RESOURCE_BARRIER barrier; 
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = pResource;
		barrier.Transition.StateBefore = resourceState;
		barrier.Transition.StateAfter = state;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		pCmdList->ResourceBarrier(1, &barrier);
	}
}

void CommittedAllocator::CreateNewPage(CommittedAllocatorBase::Page& newPage)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = newPage.data.type == ResourceType_Default ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	// 初始资源状态，如果是上传堆，则设为可读；如果是默认堆，则直接设为复制目标。
	auto initResourceState = newPage.data.type == ResourceType_Default ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
	newPage.data.resourceState = initResourceState;

	D3D12_RESOURCE_DESC cbDesc;
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	cbDesc.Width = m_blockByteSize * m_eachPageDataNum; // 总大小，单位为字节
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
		initResourceState,
		nullptr,
		IID_PPV_ARGS(&newPage.data.pResource)
	);

	std::wstring strType = newPage.data.type == ResourceType_Default ? L"Default" : L"Upload";
	std::wstring debugName = L"CBuffer Resources Pool_" + strType + L"_" + std::to_wstring(m_pages.size() - 1);
	newPage.data.pResource->SetName(debugName.c_str());
}
