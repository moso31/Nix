#pragma once
#include "DXIncludes.h"

struct AllocatorRangeInterval
{
	AllocatorRangeInterval(UINT _st, UINT _ed) : st(_st), ed(_ed) {}

	bool operator<(const AllocatorRangeInterval& other) const
	{
		if (st == other.st) return ed < other.ed;
		return st < other.st;
	}

	UINT st, ed;
};

template<typename T>
class XAllocator
{
protected:
	struct Page
	{
		Page(UINT dataSize)
		{
			freeIntervals.insert({ 0, dataSize - 1 });
		}

		std::set<AllocatorRangeInterval> freeIntervals;
		T data;
	};

	XAllocator(UINT eachPageDataNum, UINT pageNumLimit) : m_eachPageDataNum(eachPageDataNum), m_pageNumLimit(pageNumLimit) {}

	// ����һ����СΪ size ���ڴ��
	// size: Ҫ������ڴ��Ĵ�С
	// oPageIdx: ���䵽��ҳ���±�
	// oFirstIdx: ���䵽��ҳ�еĵ�һ���ڴ����±�
	// predicate: �ж�ν�ʣ�ֻ���ڴ�ν��=true��ҳ���з����ڴ档Ŀǰ������������ɸѡ��
	// onCreate��������Alloc�������µ�Page�����õĻص�������Ŀǰ���ڳ�ʼ���´�����Page�����͡�
	// onFind��������Alloc�ҵ��ɷ����ڴ�ռ䣬���õĻص���������ǰ���ã����������Ż����ˣ����ӿڿ������š�
	bool Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx,
		std::function<bool(Page& page)> predicate = [](Page& page) { return true; },
		std::function<void(Page& newPage)> onCreate = [](Page& newPage) {},
		std::function<void(Page& findPage)> onFind = [](Page& findPage) {})
	{
		// �������ҳ���С���ƣ������Ѿ����ܷ�����ҳ����Allocʧ��
		if (size > m_eachPageDataNum || m_pages.size() >= m_pageNumLimit) return false;

		for (UINT i = 0; i < (UINT)m_pages.size(); i++)
		{
			auto& page = m_pages[i];

			// ��������ν�ʣ���������ҳ
			if (!predicate(page)) continue;

			for (auto& space : page.freeIntervals)
			{
				if (space.ed - space.st + 1 >= size && space.st + size <= m_eachPageDataNum)
				{
					oPageIdx = i;
					oFirstIdx = space.st;

					// ����ҵ����ʵĿ����ڴ�
					if (space.st + size <= space.ed)
						page.freeIntervals.insert({ space.st + size, space.ed });

					page.freeIntervals.erase(space);

					onFind(page);

					return true;
				}
			}
		}

		// ���û���ҵ����ʵĿ����ڴ棬��Ҫ�·���һҳ
		auto& newPage = m_pages.emplace_back(m_eachPageDataNum);
		newPage.freeIntervals.clear();
		newPage.freeIntervals.insert({ size, m_eachPageDataNum - 1 });
		onCreate(newPage);
		CreateNewPage(newPage);
		oPageIdx = (UINT)m_pages.size() - 1;
		oFirstIdx = 0;

		return true;
	}

	virtual void CreateNewPage(Page& newPage) = 0;

	// �Ƴ� pageIdx ҳ��ģ��� start ��ʼ����Ϊ size ���ڴ��
	void Remove(UINT pageIdx, UINT start, UINT size)
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

	//void Print()
	//{
	//	for (size_t i = 0; i < m_pages.size(); i++)
	//	{
	//		auto& page = m_pages[i];
	//		std::cout << "page " << i << ":" << std::endl;
	//		for (auto& space : page.freeIntervals)
	//		{
	//			std::cout << space.st << "," << space.ed << std::endl;
	//		}
	//	}
	//}

	UINT GetPagesNum() { return (UINT)m_pages.size(); }
	UINT GetPageNumLimit() { return m_pageNumLimit; }
	UINT GetEachPageDataNum() { return m_eachPageDataNum; }

protected:
	std::vector<Page> m_pages;

	// ���allocator������ܷŶ��ٸ�page
	UINT m_pageNumLimit;

	// ÿ��page�����ɷŶ��ٸ�data
	UINT m_eachPageDataNum;
};

//int main()
//{
//	std::default_random_engine rng;
//
//	XAllocator m_allocator(1000, 5);
//	while (true)
//	{
//		// ���� true/false��true=alloc��false=remove
//		std::uniform_int_distribution<int> d0(0, 1);
//		UINT bAlloc = d0(rng);
//
//		if (bAlloc)
//		{
//			// ������� alloc
//			std::uniform_int_distribution<int> d1(0, m_allocator.GetEachPageDataNum() - 1);
//			UINT size = d1(rng);
//			UINT nouse;
//			m_allocator.Alloc(size, nouse, nouse);
//		}
//		else
//		{
//			if (m_allocator.GetPagesNum())
//			{
//				std::uniform_int_distribution<int> d1(0, m_allocator.GetPagesNum() - 1);
//				std::uniform_int_distribution<int> d2(0, m_allocator.GetEachPageDataNum() - 1);
//				//std::uniform_int_distribution<int> d3(1, m_allocator.GetEachPageDataNum());
//				std::uniform_int_distribution<int> d3(1, 20);
//				UINT pageIdx = d1(rng);
//				UINT start = d2(rng);
//				UINT size = d3(rng);
//				m_allocator.Remove(pageIdx, start, size);
//			}
//		}
//
//		m_allocator.Print();
//	}
//
//	system("pause");
//	return 0;
//}
