#include "CommittedAllocator.h"

bool CommittedAllocator::Alloc(UINT byteSize, ResourceType resourceType, D3D12_GPU_VIRTUAL_ADDRESS& oGPUVirtualAddr, UINT& oPageIdx, UINT& oPageByteOffset)
{
	size_t blockByteMask = m_blockByteSize - 1;
	UINT dataByteSize = (UINT)((byteSize + blockByteMask) & ~blockByteMask);
	UINT blockSize = dataByteSize / m_blockByteSize; // ���alloc��Ҫʹ�ö��ٸ�Block

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

	// ֻ Map Ҫ�޸ĵ���һ�μ���
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

	// ��ʼ��Դ״̬��������ϴ��ѣ�����Ϊ�ɶ��������Ĭ�϶ѣ���ֱ����Ϊ����Ŀ�ꡣ
	auto initResourceState = newPage.data.type == ResourceType_Default ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
	newPage.data.resourceState = initResourceState;

	D3D12_RESOURCE_DESC cbDesc;
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	cbDesc.Width = m_blockByteSize * m_eachPageDataNum; // �ܴ�С����λΪ�ֽ�
	cbDesc.Height = 1; // ���ڻ��������߶ȱ���Ϊ1
	cbDesc.DepthOrArraySize = 1; // ���ڻ���������ȱ���Ϊ1
	cbDesc.MipLevels = 1; // ��������MIP�ȼ�����ӦΪ1
	cbDesc.Format = DXGI_FORMAT_UNKNOWN; // ��������ʹ��DXGI��ʽ����������ΪUNKNOWN
	cbDesc.SampleDesc.Count = 1; // ���ز��������������ڻ���������Ӧ����1
	cbDesc.SampleDesc.Quality = 0; // ���ز�����������ͨ��Ϊ0
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // �������Ĳ��֣�ͨ��Ϊ������
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ��Դ�ı�־��������Ҫ��������

	HRESULT hr = m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // ��Դ����
		initResourceState,
		nullptr,
		IID_PPV_ARGS(&newPage.data.pResource)
	);

	std::wstring strType = newPage.data.type == ResourceType_Default ? L"Default" : L"Upload";
	std::wstring debugName = L"CBuffer Resources Pool_" + strType + L"_" + std::to_wstring(m_pages.size() - 1);
	newPage.data.pResource->SetName(debugName.c_str());
}
