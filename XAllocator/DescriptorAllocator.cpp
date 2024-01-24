#include "DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(ID3D12Device* pDevice) : 
	m_pDevice(pDevice),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	DescriptorAllocatorBase(DESCRIPTOR_NUM_PER_HEAP_MAXLIMIT, 100)
{
	// 创建一个 shader-visible 的描述符堆，用于渲染前每帧提交。
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = DESCRIPTOR_NUM_PER_HEAP_MAXLIMIT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_renderHeap));
}

// 分配一个大小为 size 的内存块
// size: 要分配的内存块的大小
// oPageIdx: 分配到的页的下标
// oFirstIdx: 分配到的页中的第一个内存块的下标
bool DescriptorAllocator::Alloc(DescriptorType type, UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	auto predicate = [type](Page& page){
		return page.data.type == type;
	};

	auto onCreate = [type](Page& newPage) {
		newPage.data.type = type;
	};

	if (DescriptorAllocatorBase::Alloc(size, oPageIdx, oFirstIdx, predicate, onCreate))
	{
		auto& pDescriptor = m_pages[oPageIdx].data;
		oHandle = pDescriptor.data->GetCPUDescriptorHandleForHeapStart();
		oHandle.ptr += oFirstIdx * m_descriptorByteSize;
		return true;
	}

	return false;
}

void DescriptorAllocator::Remove(UINT pageIdx, UINT start, UINT size)
{
	auto& freeIntervals = m_pages[pageIdx].freeIntervals;

	UINT end = min(start + size - 1, m_eachPageDataNum - 1);

	AllocatorRangeInterval adjust(start, end);
	std::set<AllocatorRangeInterval> removing;
	for (auto& space : freeIntervals)
	{
		bool bCombine = false;
		if (space.st >= start && space.ed <= end)
		{
			// 如果 space 是子集，删除
			removing.insert(space);
		}
		else if (space.st <= end && start <= space.ed)
		{
			// 如果 space 是交集，合并
			removing.insert(space);
			bCombine = true;
		}
		else if (space.st < start || space.ed > end)
		{
			// 如果 space 是父集，什么都不做
		}
		else bCombine = true;

		if (bCombine)
		{
			adjust.st = min(adjust.st, space.st);
			adjust.ed = max(adjust.ed, space.ed);
		}
	}

	for (auto& space : removing) freeIntervals.erase(space);

	// 如果 adjust 和 m_freeInterval 形成连号，需要再合并一次。
	removing.clear();
	for (auto& space : freeIntervals)
	{
		if (space.st == adjust.ed + 1)
		{
			adjust.ed = space.ed;
			removing.insert(space);
		}
		else if (space.ed == adjust.st - 1)
		{
			adjust.st = space.st;
			removing.insert(space);
		}
	}

	freeIntervals.insert(adjust);
	for (auto& space : removing) freeIntervals.erase(space);
}

void DescriptorAllocator::CreateNewPage(DescriptorAllocatorBase::Page& newPage)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // cpu heap, 默认 FLAG_NONE = non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = m_eachPageDataNum;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // 此 allocator 只支持 CBVSRVUAV 这一种类型.

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newPage.data.data));
}

UINT DescriptorAllocator::AppendToRenderHeap(const size_t* cpuHandles, const size_t cpuHandlesSize)
{
	UINT firstOffsetIndex = m_currentOffset;

	for (size_t i = 0; i < cpuHandlesSize; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle;
		srcHandle.ptr = cpuHandles[i];

		// 计算新的 ring buffer 偏移量
		UINT heapOffset = m_currentOffset * m_descriptorByteSize;
		D3D12_CPU_DESCRIPTOR_HANDLE destHandle = m_renderHeap->GetCPUDescriptorHandleForHeapStart();
		destHandle.ptr += heapOffset;

		// 拷贝描述符
		m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// 更新偏移量
		m_currentOffset = (m_currentOffset + 1) % DESCRIPTOR_NUM_PER_HEAP_MAXLIMIT;
	}

	return firstOffsetIndex;
}
