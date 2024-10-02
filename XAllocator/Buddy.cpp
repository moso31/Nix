#include "Buddy.h"

using namespace ccmem;

ccmem::BuddyAllocator::BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize) :
	MIN_LV(blockByteSize),
	MAX_LV(fullByteSize)
{
	// 计算最小和最大级别的log2值 和 LV_NUM
	for (MIN_LV_LOG2 = 0; uint32_t(1 << MIN_LV_LOG2) < MIN_LV; ++MIN_LV_LOG2);
	for (MAX_LV_LOG2 = 0; uint32_t(1 << MAX_LV_LOG2) < MAX_LV; ++MAX_LV_LOG2);
	LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1;

	// 初始化时自动创建一个Allocator 
	AddAllocatorInternal();
}

void ccmem::BuddyAllocator::Alloc(uint32_t byteSize, const std::function<void(const BuddyTaskResult&)>& callback)
{
	BuddyTask task;
	task.byteSize = byteSize;
	task.pFreeMem = nullptr;
	task.pCallBack = callback;

	m_mutex.lock();
	m_taskList.push_back(task);
	m_mutex.unlock();
}

void ccmem::BuddyAllocator::Free(uint8_t* pFreeMem)
{
	BuddyTask task;
	task.byteSize = 0;
	task.pFreeMem = pFreeMem;
	task.pCallBack = nullptr;

	m_mutex.lock();
	m_taskList.push_back(task);
	m_mutex.unlock();
}

void ccmem::BuddyAllocator::ExecuteTasks()
{
	m_mutex.lock();
	std::list<BuddyTask> taskList;
	taskList.swap(m_taskList);
	m_mutex.unlock();

	// 将低优先级任务队列合并到任务队列中，但放在最后执行
	taskList.splice(taskList.end(), m_lowPriorTaskList);
	m_lowPriorTaskList.clear();

	for (auto& task : taskList)
	{
		if (!task.pFreeMem) // 分配
		{
			BuddyTaskResult taskResult;
			task.state = TryAlloc(task, taskResult);

			// 如果分配成功，触发回调函数
			if (task.state == BuddyTask::State::Success)
				task.pCallBack(taskResult);
		}
		else // 释放
		{
			task.state = TryFree(task);
		}
	}

	// 删除成功的Task，以及释放时没找到对应内存块的Task
	taskList.remove_if([](const BuddyTask& task) {
		return task.state == BuddyTask::State::Success || task.state == BuddyTask::State::Failed_Free_NotFind;
		});

	// 将失败的Task 转移至低优先级任务队列
	m_lowPriorTaskList.swap(taskList);
}

void ccmem::BuddyAllocator::Print()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pAllocator : m_allocatorPages)
	{
		std::cout << "Allocator: " << pAllocator->GetPageID() << std::endl;
		pAllocator->Print();
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

uint32_t ccmem::BuddyAllocator::GetAlignedByteSize(uint32_t byteSize)
{
	uint32_t alignedSize = 1;
	while (alignedSize < byteSize) alignedSize <<= 1;
	return alignedSize;
}

BuddyAllocatorPage* ccmem::BuddyAllocator::AddAllocatorInternal()
{
	// 这里的锁本来不应该和ExecuteTasks共用，
	// 不够调用这个方法的机会并不多，共用影响也不大
	std::lock_guard<std::mutex> lock(m_mutex);

	BuddyAllocatorPage* pAllocator = new BuddyAllocatorPage(this);
	m_allocatorPages.push_back(pAllocator);
	return pAllocator;
}

BuddyTask::State ccmem::BuddyAllocator::TryAlloc(const BuddyTask& task, BuddyTaskResult& oTaskResult)
{
	BuddyTask::State result;
	for (auto& pAllocator : m_allocatorPages)
	{
		if (pAllocator->m_freeByteSize >= task.byteSize)
		{
			result = pAllocator->AllocSync(task, oTaskResult.pMemory);

			// 如果返回未知错误，不接受，直接让它崩溃
			assert(result != BuddyTask::State::Failed_Unknown);

			// 分配成功时结束循环
			if (result == BuddyTask::State::Success)
			{
				oTaskResult.pAllocator = pAllocator;
				return result;
			}
		}
	}

	// 如果走到这里，说明所有的Allocator都满了，创建一个新的Allocator
	BuddyAllocatorPage* pNewAllocator = AddAllocatorInternal();
	result = pNewAllocator->AllocSync(task, oTaskResult.pMemory);
	if (result == BuddyTask::State::Success)
	{
		oTaskResult.pAllocator = pNewAllocator;
		return result;
	}

	// 如果返回未知错误，不接受，直接让它崩溃
	assert(result != BuddyTask::State::Failed_Unknown);

	oTaskResult.pMemory = nullptr;
	oTaskResult.pAllocator = nullptr;
	return result;
}

BuddyTask::State ccmem::BuddyAllocator::TryFree(const BuddyTask& task)
{
	BuddyTask::State result = BuddyTask::State::Pending;
	for (auto& pAllocator : m_allocatorPages)
	{
		result = pAllocator->FreeSync(task);
		if (result == BuddyTask::State::Success)
			return result;
	}

	return result;
}

ccmem::BuddyAllocatorPage::BuddyAllocatorPage(BuddyAllocator* pOwner) :
	m_pOwner(pOwner)
{
	static uint32_t s_pageID = 0;
	assert(s_pageID < UINT_MAX);
	m_pageID = s_pageID++;

	m_freeByteSize = pOwner->GetMaxLevel();
	m_pMem = new uint8_t[m_freeByteSize];

	m_freeList.resize(pOwner->GetLevelNum());
	m_freeList[0].push_back(m_pMem);

	m_usedList.resize(pOwner->GetLevelNum());
}

ccmem::BuddyAllocatorPage::~BuddyAllocatorPage()
{
	for (int i = 0; i < m_freeList.size(); i++) m_freeList[i].clear();
	for (int i = 0; i < m_usedList.size(); i++) m_usedList[i].clear();
	delete[] m_pMem;
}

BuddyTask::State ccmem::BuddyAllocatorPage::AllocSync(const BuddyTask& task, uint8_t*& pAllocMem)
{
	// 确定byteSize对应的最小内存块级别
	int newBlockLevel = m_pOwner->GetLevel(task.byteSize);

	// 从该级别开始向上查找，找到可用的内存块
	for (int i = newBlockLevel; i >= 0; i--)
	{
		if (!m_freeList[i].empty())
		{
			pAllocMem = AllocInternal(newBlockLevel, i);
			if (pAllocMem)
			{
				m_freeByteSize -= m_pOwner->GetAlignedByteSize(task.byteSize);
				return BuddyTask::State::Success; // 分配成功
			}
			else
			{
				pAllocMem = nullptr;
				return BuddyTask::State::Failed_Unknown; // 分配失败，目前暂时没有走到这里的情况，先返回个未知错误吧
			}
		}
	}

	// 如果走到这里，说明内存已经满了
	pAllocMem = nullptr;
	return BuddyTask::State::Failed_Alloc_FullMemory;
}

BuddyTask::State ccmem::BuddyAllocatorPage::FreeSync(const BuddyTask& task)
{
	// 从used列表中找到该内存块
	for (uint32_t i = 0; i < m_pOwner->GetLevelNum(); i++)
	{
		auto it = std::find(m_usedList[i].begin(), m_usedList[i].end(), task.pFreeMem);
		if (it != m_usedList[i].end())
		{
			m_usedList[i].erase(it);
			m_freeList[i].push_back(task.pFreeMem);
			FreeInternal(std::prev(m_freeList[i].end()), i);

			m_freeByteSize += 1 << (m_pOwner->GetMaxLevelLog2() - i);
			return BuddyTask::State::Success;
		}
	}

	// 如果走到这里，说明该内存块不在used列表中
	return BuddyTask::State::Failed_Free_NotFind;
}

void ccmem::BuddyAllocatorPage::Print()
{
	for (uint32_t i = 0; i < m_pOwner->GetLevelNum(); i++)
	{
		uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - i);
		std::cout << "  Free " << i << "(size=" << blockSize << "): ";

		for (auto& freeBlock : m_freeList[i])
		{
			uintptr_t offset = reinterpret_cast<uintptr_t>(freeBlock) - reinterpret_cast<uintptr_t>(m_pMem);
			std::cout << offset << " ";
		}

		std::cout << std::endl;
	};

	for (uint32_t i = 0; i < m_pOwner->GetLevelNum(); i++)
	{
		uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - i);
		std::cout << "  used " << i << "(size=" << blockSize << "): ";

		for (auto& usedBlock : m_usedList[i])
		{
			uintptr_t offset = reinterpret_cast<uintptr_t>(usedBlock) - reinterpret_cast<uintptr_t>(m_pMem);
			std::cout << offset << " ";
		}

		std::cout << std::endl;
	};
}

uint8_t* ccmem::BuddyAllocatorPage::AllocInternal(uint32_t destLevel, uint32_t srcLevel)
{
	assert(srcLevel < m_pOwner->GetLevelNum());

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
		uint32_t halfBlockSize = 1 << (m_pOwner->GetMaxLevelLog2() - srcLevel - 1);
		m_freeList[srcLevel + 1].push_back(p);
		m_freeList[srcLevel + 1].push_back(p + halfBlockSize);

		// 递归查下一级别，直到找到合适的内存块
		return AllocInternal(destLevel, srcLevel + 1);
	}

	return nullptr;
}

void ccmem::BuddyAllocatorPage::FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level)
{
	if (level == 0) return;

	// itMem 指向要释放的内存块
	uint8_t* pMem = *itMem;

	// 检查该内存块的buddy是否都在free列表中，如果存在就合并成一个更大的块
	// 使用XOR操作找到buddy
	uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - level);
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