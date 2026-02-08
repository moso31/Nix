#include "RTVAllocator.h"

RTVAllocator::RTVAllocator(ID3D12Device* pDevice) : 
	m_pDevice(pDevice),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	RTVAllocatorBase(DESCRIPTOR_NUM_PER_HEAP_RTV, 100)
{
}

bool RTVAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	UINT oPageIdx, oFirstIdx;
	return Alloc(1, oPageIdx, oFirstIdx, oHandle);
}

// 分配一个大小为 size 的内存块
// size: 要分配的内存块的大小
// oPageIdx: 分配到的页的下标
// oFirstIdx: 分配到的页中的第一个内存块的下标
bool RTVAllocator::Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	if (RTVAllocatorBase::Alloc(size, oPageIdx, oFirstIdx))
	{
		auto& pDescriptorHeap = m_pages[oPageIdx].data;
		oHandle = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		oHandle.ptr += oFirstIdx * m_descriptorByteSize;
		return true;
	}

	return false;
}

void RTVAllocator::Remove(UINT pageIdx, UINT start, UINT size)
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

void RTVAllocator::CreateNewPage(RTVAllocatorBase::Page& newPage)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // cpu heap, 默认 FLAG_NONE = non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = m_eachPageDataNum;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // 此 allocator 只支持 RTV 这一种类型.

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newPage.data));
}

void RTVAllocator::Clear()
{
	for (UINT i = 0; i < (UINT)m_pages.size(); ++i)
	{
		ClearPage(i);

		auto& page = m_pages[i];
		if (page.data)
			page.data->Release();
	}

	m_pages.clear();
}
