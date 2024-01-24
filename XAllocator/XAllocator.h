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

	// 分配一个大小为 size 的内存块
	// size: 要分配的内存块的大小
	// oPageIdx: 分配到的页的下标
	// oFirstIdx: 分配到的页中的第一个内存块的下标
	// predicate: 判断谓词，只能在此谓词=true的页面中分配内存。目前用来进行类型筛选。
	// onCreate：若本次Alloc创建了新的Page，调用的回调函数。目前用于初始化新创建的Page的类型。
	// onFind：若本次Alloc找到可分配内存空间，调用的回调函数。以前有用，后来被我优化掉了，但接口可以留着。
	bool Alloc(UINT size, UINT& oPageIdx, UINT& oFirstIdx,
		std::function<bool(Page& page)> predicate = [](Page& page) { return true; },
		std::function<void(Page& newPage)> onCreate = [](Page& newPage) {},
		std::function<void(Page& findPage)> onFind = [](Page& findPage) {})
	{
		// 如果超过页面大小限制，或者已经不能分配新页，则Alloc失败
		if (size > m_eachPageDataNum || m_pages.size() >= m_pageNumLimit) return false;

		for (UINT i = 0; i < (UINT)m_pages.size(); i++)
		{
			auto& page = m_pages[i];

			// 若不满足谓词，则跳过此页
			if (!predicate(page)) continue;

			for (auto& space : page.freeIntervals)
			{
				if (space.ed - space.st + 1 >= size && space.st + size <= m_eachPageDataNum)
				{
					oPageIdx = i;
					oFirstIdx = space.st;

					// 如果找到合适的空闲内存
					if (space.st + size <= space.ed)
						page.freeIntervals.insert({ space.st + size, space.ed });

					page.freeIntervals.erase(space);

					onFind(page);

					return true;
				}
			}
		}

		// 如果没有找到合适的空闲内存，需要新分配一页
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

	// 移除 pageIdx 页面的，从 start 开始长度为 size 的内存块
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

	// 这个allocator中最多能放多少个page
	UINT m_pageNumLimit;

	// 每个page中最多可放多少个data
	UINT m_eachPageDataNum;
};

//int main()
//{
//	std::default_random_engine rng;
//
//	XAllocator m_allocator(1000, 5);
//	while (true)
//	{
//		// 生成 true/false，true=alloc，false=remove
//		std::uniform_int_distribution<int> d0(0, 1);
//		UINT bAlloc = d0(rng);
//
//		if (bAlloc)
//		{
//			// 随机生成 alloc
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
