#include "MemAllocTest.h"

using namespace ccmem;

void ccmem::BuddyAllocatorTaskQueue::Push(BuddyAllocatorTask task)
{
	m_taskList.push_back(task);
}

void ccmem::BuddyAllocatorTaskQueue::ExecuteTasks()
{
	for (auto& task : m_taskList)
	{
		if (!task.pMem)
		{
			uint8_t* pMemory = task.pAllocator->AllocSync(task.byteSize);
			task.callback(pMemory);
		}
		else
			task.pAllocator->FreeSync(task.pMem);
	}

	m_taskList.clear();
}

ccmem::BuddyAllocator::BuddyAllocator(BuddyAllocatorTaskQueue* pQueue) :
	m_pTaskQueue(pQueue)
{
	m_pMem = new uint8_t[MAX_LV];

	m_freeList[0].push_back(m_pMem);
}

ccmem::BuddyAllocator::~BuddyAllocator()
{
	for (int i = 0; i < LV_NUM; i++) m_freeList[i].clear();
	delete[] m_pMem;
}

uint8_t* ccmem::BuddyAllocator::AllocSync(uint32_t byteSize)
{
	// 确定byteSize对应的最小内存块级别
	int newBlockLevel = GetLevel(byteSize);

	// 从该级别开始向上查找，找到可用的内存块
	for (int i = newBlockLevel; i >= 0; i--)
	{
		if (!m_freeList[i].empty())
		{
			return AllocInternal(newBlockLevel, i);
		}
	}

	return nullptr;
}

void ccmem::BuddyAllocator::FreeSync(uint8_t* pMem)
{
	// 从used列表中找到该内存块
	for (int i = 0; i < LV_NUM; i++)
	{
		auto it = std::find(m_usedList[i].begin(), m_usedList[i].end(), pMem);
		if (it != m_usedList[i].end())
		{
			m_usedList[i].erase(it);
			m_freeList[i].push_back(pMem);
			FreeInternal(std::prev(m_freeList[i].end()), i);
			return;
		}
	}
}

void ccmem::BuddyAllocator::Print()
{
	for (int i = 0; i < LV_NUM; i++)
	{
		int blockSize = 1 << (MAX_LV_LOG2 - i);
		std::cout << "  Free " << i << "(size=" << blockSize << "): ";

		for (auto& freeBlock : m_freeList[i])
		{
			int offset = reinterpret_cast<uintptr_t>(freeBlock) - reinterpret_cast<uintptr_t>(m_pMem);
			std::cout << offset << " ";
		}

		std::cout << std::endl;
	};

	for (int i = 0; i < LV_NUM; i++)
	{
		int blockSize = 1 << (MAX_LV_LOG2 - i);
		std::cout << "  used " << i << "(size=" << blockSize << "): ";

		for (auto& usedBlock : m_usedList[i])
		{
			int offset = reinterpret_cast<uintptr_t>(usedBlock) - reinterpret_cast<uintptr_t>(m_pMem);
			std::cout << offset << " ";
		}

		std::cout << std::endl;
	};
}

void ccmem::BuddyAllocator::AllocAsync(uint32_t byteSize, std::function<void(uint8_t*)> callback)
{
	BuddyAllocatorTask task;
	task.pAllocator = this;
	task.byteSize = byteSize;
	task.callback = callback;
	task.pMem = nullptr;

	m_pTaskQueue->Push(task);
}

void ccmem::BuddyAllocator::FreeAsync(uint8_t* pMem)
{
	BuddyAllocatorTask task;
	task.pAllocator = this;
	task.byteSize = 0;
	task.pMem = pMem;
	task.callback = nullptr;

	m_pTaskQueue->Push(task);
}

uint8_t* ccmem::BuddyAllocator::AllocInternal(uint32_t destLevel, uint32_t srcLevel)
{
	assert(srcLevel < LV_NUM);

	// 如果有该级别的内存块可用，直接使用
	if (destLevel == srcLevel)
	{
		// 获取该列表的第一个元素，即可用的内存块
		uint8_t* p = m_freeList[destLevel].front();
		m_freeList[destLevel].pop_front(); // 从free列表中删除该元素
		m_usedList[destLevel].push_back(p); // 在used列表中添加该元素
		return p;
	}

	// 整体机制决定了srcLevel里一定有合适的内存块（除非内存满了）
	if (!m_freeList[srcLevel].empty())
	{
		// 从该级别的空闲列表中取出一个内存块
		uint8_t* p = m_freeList[srcLevel].front();
		m_freeList[srcLevel].pop_front();

		// 将该内存块一分为二，都放入下一级别的空闲列表
		uint32_t halfBlockSize = 1 << (MAX_LV_LOG2 - srcLevel - 1);
		m_freeList[srcLevel + 1].push_back(p);
		m_freeList[srcLevel + 1].push_back(p + halfBlockSize);

		// 递归查下一级别，直到找到合适的内存块
		return AllocInternal(destLevel, srcLevel + 1);
	}

	return nullptr;
}

void ccmem::BuddyAllocator::FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level)
{
	// itMem 指向要释放的内存块
	uint8_t* pMem = *itMem;

	// 检查该内存块的buddy是否都在free列表中，如果存在就合并成一个更大的块
	// 使用XOR操作找到buddy
	uint32_t blockSize = 1 << (MAX_LV_LOG2 - level);
	uintptr_t offset = reinterpret_cast<uintptr_t>(pMem) - reinterpret_cast<uintptr_t>(m_pMem);
	uintptr_t buddyOffset = offset ^ blockSize;
	uint8_t* pBuddyMem = m_pMem + buddyOffset;

	// 从free列表中找到buddy
	auto itBuddyMem = std::find(m_freeList[level].begin(), m_freeList[level].end(), pBuddyMem);
	if (itBuddyMem != m_freeList[level].end())
	{
		// 合并
		m_freeList[level].erase(itMem);
		m_freeList[level].erase(itBuddyMem);

		uint8_t* pMergedMem = std::min(pMem, pBuddyMem);
		m_freeList[level - 1].push_back(pMergedMem); 

		// 递归处理更大一级的内存块
		FreeInternal(std::prev(m_freeList[level - 1].end()), level - 1);
	}
}

uint32_t ccmem::BuddyAllocator::GetLevel(uint32_t byteSize)
{
	assert(byteSize <= MAX_LV);

	byteSize--;
	uint32_t level = 0;
	while (byteSize)
	{
		byteSize >>= 1;
		level++;
	};

	return MAX_LV_LOG2 - std::max(level, MIN_LV_LOG2);
}
