#include "DSVAllocator.h"

DSVAllocator::DSVAllocator(ID3D12Device* pDevice) : 
	m_pDevice(pDevice),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	DSVAllocatorBase(DESCRIPTOR_NUM_PER_HEAP_DSV, 100)
{
}

bool DSVAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	UINT oPageIdx, oFirstIdx;
	return Alloc(1, oPageIdx, oFirstIdx, oHandle);
}

// ����һ����СΪ size ���ڴ��
// size: Ҫ������ڴ��Ĵ�С
// oPageIdx: ���䵽��ҳ���±�
// oFirstIdx: ���䵽��ҳ�еĵ�һ���ڴ����±�
bool DSVAllocator::Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	if (DSVAllocatorBase::Alloc(size, oPageIdx, oFirstIdx))
	{
		auto& pDescriptor = m_pages[oPageIdx].data;
		oHandle = pDescriptor->GetCPUDescriptorHandleForHeapStart();
		oHandle.ptr += oFirstIdx * m_descriptorByteSize;
		return true;
	}

	return false;
}

void DSVAllocator::Remove(UINT pageIdx, UINT start, UINT size)
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
			// ��� space ���Ӽ���ɾ��
			removing.insert(space);
		}
		else if (space.st <= end && start <= space.ed)
		{
			// ��� space �ǽ������ϲ�
			removing.insert(space);
			bCombine = true;
		}
		else if (space.st < start || space.ed > end)
		{
			// ��� space �Ǹ�����ʲô������
		}
		else bCombine = true;

		if (bCombine)
		{
			adjust.st = min(adjust.st, space.st);
			adjust.ed = max(adjust.ed, space.ed);
		}
	}

	for (auto& space : removing) freeIntervals.erase(space);

	// ��� adjust �� m_freeInterval �γ����ţ���Ҫ�ٺϲ�һ�Ρ�
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

void DSVAllocator::CreateNewPage(DSVAllocatorBase::Page& newPage)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // cpu heap, Ĭ�� FLAG_NONE = non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = m_eachPageDataNum;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // �� allocator ֻ֧�� DSV ��һ������.

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newPage.data));
}

void DSVAllocator::Clear()
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
