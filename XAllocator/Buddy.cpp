#include "Buddy.h"

using namespace ccmem;

ccmem::BuddyTaskResult::BuddyTaskResult(const BuddyTask& connectTask)
{
	selfID = ccmem::GenerateUniqueTaskID();
	connectTaskID = connectTask.selfID;
}

ccmem::BuddyTaskResult::~BuddyTaskResult()
{
	if (pTaskContext)
	{
		delete[] pTaskContext;
		pTaskContext = nullptr;
	}
}

ccmem::BuddyTask::BuddyTask()
{
	selfID = ccmem::GenerateUniqueTaskID();
}

ccmem::BuddyAllocator::BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize, const std::wstring& name) :
	m_name(name),
	MIN_LV(blockByteSize),
	MAX_LV(fullByteSize)
{
	// 计算最小和最大级别的log2值 和 LV_NUM
	for (MIN_LV_LOG2 = 0; uint32_t(1 << MIN_LV_LOG2) < MIN_LV; ++MIN_LV_LOG2);
	for (MAX_LV_LOG2 = 0; uint32_t(1 << MAX_LV_LOG2) < MAX_LV; ++MAX_LV_LOG2);
	LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1;
}

ccmem::BuddyAllocator::~BuddyAllocator()
{
	for (auto& pAllocator : m_allocatorPages)
	{
		delete pAllocator;
	}
	m_allocatorPages.clear();
}

void ccmem::BuddyAllocator::AddAllocTask(uint32_t byteSize, void* pTaskContext, uint32_t pTaskContextSize, const std::function<void(const BuddyTaskResult&)>& callback)
{
	assert(byteSize > 0);

	BuddyTask task;
	task.byteSize = byteSize;
	task.pCallBack = callback;
	task.state = BuddyTask::State::Pending;
	task.pTaskContextSize = pTaskContextSize;
	task.pTaskContext = new uint8_t[pTaskContextSize];
	memcpy(task.pTaskContext, pTaskContext, pTaskContextSize);

	m_mutex.lock();
	m_taskList.push_back(task);
	m_mutex.unlock();
}

void ccmem::BuddyAllocator::AddFreeTask(BuddyAllocatorPage* pAllocator, uint32_t pFreeMem)
{
	BuddyTask task;
	task.offset = pFreeMem;
	task.pFreeAllocator = pAllocator;
	task.pCallBack = nullptr; // no use when free

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
		if (task.pCallBack) // is alloc
		{
			BuddyTaskResult taskResult(task);
			task.state = TryAlloc(task, taskResult.memData);

			// 如果分配成功，触发回调函数
			if (task.state == BuddyTask::State::Success)
			{
				// 移交pTaskContext所有权给taskResult
				taskResult.pTaskContext = task.pTaskContext;
				task.pTaskContext = nullptr; // 防止重复释放
				task.pCallBack(taskResult);
				task.pCallBack = nullptr;
			}
		}
		else // is free
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
	}

	return MAX_LV_LOG2 - std::max(level, MIN_LV_LOG2);
}

uint32_t ccmem::BuddyAllocator::GetAlignedByteSize(uint32_t byteSize)
{
	uint32_t alignedSize = 1;
	while (alignedSize < byteSize) alignedSize <<= 1;
	return alignedSize;
}

uint32_t ccmem::BuddyAllocator::GetByteSizeFromLevel(uint32_t level)
{
	return 1 << (MAX_LV_LOG2 - level);
}

BuddyAllocatorPage* ccmem::BuddyAllocator::AddAllocatorInternal()
{
	// 这里的锁本来不应该和ExecuteTasks共用，
	// 不够调用这个方法的机会并不多，共用影响也不大
	std::lock_guard<std::mutex> lock(m_mutex);

	BuddyAllocatorPage* pAllocator = new BuddyAllocatorPage(this);
	m_allocatorPages.push_back(pAllocator);

	OnAllocatorAdded(pAllocator);

	return pAllocator;
}

BuddyTask::State ccmem::BuddyAllocator::TryAlloc(const BuddyTask& task, XBuddyTaskMemData& oTaskMemData)
{
	BuddyTask::State result;
	for (auto& pAllocator : m_allocatorPages)
	{
		if (pAllocator->m_freeByteSize >= task.byteSize)
		{
			result = pAllocator->AllocSync(task, oTaskMemData.byteOffset);
			// 如果返回未知错误，不接受，直接让它崩溃
			assert(result != BuddyTask::State::Failed_Unknown);

			// 分配成功时结束循环
			if (result == BuddyTask::State::Success)
			{
				oTaskMemData.pAllocator = pAllocator;
				return result;
			}
		}
	}

	// 如果走到这里，说明所有的Allocator都满了，创建一个新的Allocator
	BuddyAllocatorPage* pNewAllocator = AddAllocatorInternal();
	result = pNewAllocator->AllocSync(task, oTaskMemData.byteOffset);
	if (result == BuddyTask::State::Success)
	{
		oTaskMemData.pAllocator = pNewAllocator;
		return result;
	}

	// 如果返回未知错误，不接受，直接让它崩溃
	assert(result != BuddyTask::State::Failed_Unknown);
	return result;
}

BuddyTask::State ccmem::BuddyAllocator::TryFree(const BuddyTask& task)
{
	BuddyTask::State result = task.pFreeAllocator->FreeSync(task.offset);
	if (result == BuddyTask::State::Success)
		return result;

	return result;
}

ccmem::BuddyAllocatorPage::BuddyAllocatorPage(BuddyAllocator* pOwner) :
	m_pOwner(pOwner)
{
	static uint32_t s_pageID = 0;
	assert(s_pageID < UINT_MAX);
	m_pageID = s_pageID++;

	m_freeByteSize = pOwner->GetMaxLevel();

	BuddyMemoryBlock block;
	block.level = 0;
	block.bUsed = false;
	m_memory.insert({ 0, block });
}

ccmem::BuddyAllocatorPage::~BuddyAllocatorPage()
{
}

BuddyTask::State ccmem::BuddyAllocatorPage::AllocSync(const BuddyTask& task, uint32_t& oByteOffset)
{
	// 定级：确定byteSize对应的最小内存块级别
	uint32_t newBlockLevel = m_pOwner->GetLevel(task.byteSize);

	bool hasMinBlock = false;
	uint32_t minBlockByteOffset;
	BuddyMemoryBlock pMinBlock;

	// 检查内存块，如果已经有同级内存块，可直接使用
	for (auto& [byteOffset, blockData] : m_memory)
	{
		if (!blockData.bUsed)
		{
			if (blockData.level == newBlockLevel)
			{
				blockData.bUsed = true;
				oByteOffset = byteOffset;
				return BuddyTask::State::Success;
			}

			// 如果没有同级内存块，找到一个比定级更大，但最小的内存块（等级越小，内存块越大）
			if (blockData.level < newBlockLevel) 
			{
				if (!hasMinBlock || blockData.level > pMinBlock.level)
				{
					hasMinBlock = true;
					minBlockByteOffset = byteOffset;
					pMinBlock = blockData;
				}
			}
		}
	}

	// 如果找到了大于定级的最小内存块，将其分割成定级的内存块
	if (hasMinBlock)
	{
		m_memory.erase(minBlockByteOffset);

		// for 逐级拆分
		for (uint32_t i = pMinBlock.level; i < newBlockLevel; i++)
		{
			// 永远只将右子块拆分出去
			BuddyMemoryBlock halfBlock;
			halfBlock.level = i + 1;
			halfBlock.bUsed = false;

			uint32_t halfByteOffset = minBlockByteOffset + m_pOwner->GetByteSizeFromLevel(i + 1);
			m_memory.insert({ halfByteOffset, halfBlock });

			// 拆分到定级时，将左子块标记为已使用
			if (i + 1 == newBlockLevel)
			{
				halfBlock.bUsed = true;
				oByteOffset = minBlockByteOffset;
				m_memory.insert({ oByteOffset , halfBlock });

				// 分配成功，并且后面没有逻辑，可以直接返回了
				return BuddyTask::State::Success; 
			}
		}
	}
	
	// 找不到说明内存已满
	return BuddyTask::State::Failed_Alloc_FullMemory; 
}

BuddyTask::State ccmem::BuddyAllocatorPage::FreeSync(const uint32_t& freeByteOffset)
{
	auto it = m_memory.find(freeByteOffset);
	if (it == m_memory.end())
	{
		return BuddyTask::State::Failed_Free_NotFind;
	}

	if (it->second.bUsed == false)
	{
		return BuddyTask::State::Failed_Unknown;
	}

	it->second.bUsed = false;

	uint32_t currentOffset = freeByteOffset;  // 用可变的局部变量

	// 逐级合并
	for (uint32_t i = it->second.level; i > 0; i--)
	{
		// 当前内存块的字节大小
		uint32_t freeByteSize = m_pOwner->GetByteSizeFromLevel(i);

		// xor 计算伙伴内存偏移量
		uint32_t buddyByteOffset = currentOffset ^ freeByteSize;

		uint32_t parentByteOffset = std::min(currentOffset, buddyByteOffset);

		auto it = m_memory.find(buddyByteOffset);
		if (it == m_memory.end())
			throw std::runtime_error("buddy not found."); // 直接抛异常，不应该找不到buddy

		// buddy必须同级，并且没有被占用，才能合并
		if (it->second.level != i || it->second.bUsed == true) 
			break;

		// 合并buddy
		m_memory.erase(currentOffset);
		m_memory.erase(buddyByteOffset);

		BuddyMemoryBlock block;
		block.level = i - 1;
		block.bUsed = false;
		m_memory.insert({ parentByteOffset, block });

		currentOffset = parentByteOffset;  // 父级buddy，继续迭代
	}

	return BuddyTask::State::Success;
}

void ccmem::BuddyAllocatorPage::Print()
{
	printf("memory:");
	for (auto& [x, y] : m_memory)
	{
		printf("%d", x);
		printf(y.bUsed ? "T" : "F");
		printf(" ");
	}

	printf("\n");
}
