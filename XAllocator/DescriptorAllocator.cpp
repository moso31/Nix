#include "DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(ID3D12Device* pDevice) : 
	DescriptorAllocator(pDevice, DESCRIPTOR_NUM_PER_HEAP_MAXLIMIT, 100)
{
}

DescriptorAllocator::DescriptorAllocator(ID3D12Device* pDevice, UINT pageNumLimit, UINT pageSizeLimit) :
	m_pDevice(pDevice),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	DescriptorAllocatorBase(pageNumLimit, pageSizeLimit)
{
}

// ����һ����СΪ size ���ڴ��
// size: Ҫ������ڴ��Ĵ�С
// oPageIdx: ���䵽��ҳ���±�
// oFirstIdx: ���䵽��ҳ�еĵ�һ���ڴ����±�
// oHandles: ���ط�����׸� cpu descriptor handle �ĵ�ַ
bool DescriptorAllocator::Alloc(DescriptorType type, UINT size, UINT& oPageIdx, UINT& oFirstIdx, D3D12_CPU_DESCRIPTOR_HANDLE& oHandles)
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
		oHandles = pDescriptor.data->GetCPUDescriptorHandleForHeapStart();
		oHandles.ptr += oFirstIdx * m_descriptorByteSize;
		return true;
	}

	return false;
}

bool DescriptorAllocator::Alloc(DescriptorType type, D3D12_CPU_DESCRIPTOR_HANDLE& oHandle)
{
	UINT nouse[2];
	return Alloc(type, 1, nouse[0], nouse[1], oHandle);
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

void DescriptorAllocator::CreateNewPage(DescriptorAllocatorBase::Page& newPage)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // cpu heap, Ĭ�� FLAG_NONE = non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = m_eachPageDataNum;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �� allocator ֻ֧�� CBVSRVUAV ��һ������.

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newPage.data.data));
}
