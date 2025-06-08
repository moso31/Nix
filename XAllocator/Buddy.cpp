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

ccmem::BuddyAllocator::BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize) :
	MIN_LV(blockByteSize),
	MAX_LV(fullByteSize)
{
	// ������С����󼶱��log2ֵ �� LV_NUM
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

	// �������ȼ�������кϲ�����������У����������ִ��
	taskList.splice(taskList.end(), m_lowPriorTaskList);
	m_lowPriorTaskList.clear();

	for (auto& task : taskList)
	{
		if (task.pCallBack) // is alloc
		{
			BuddyTaskResult taskResult(task);
			task.state = TryAlloc(task, taskResult.memData);

			// �������ɹ��������ص�����
			if (task.state == BuddyTask::State::Success)
			{
				// �ƽ�pTaskContext����Ȩ��taskResult
				taskResult.pTaskContext = task.pTaskContext;
				task.pTaskContext = nullptr; // ��ֹ�ظ��ͷ�
				task.pCallBack(taskResult);
			}
		}
		else // is free
		{
			task.state = TryFree(task);
		}
	}

	// ɾ���ɹ���Task���Լ��ͷ�ʱû�ҵ���Ӧ�ڴ���Task
	taskList.remove_if([](const BuddyTask& task) {
		return task.state == BuddyTask::State::Success || task.state == BuddyTask::State::Failed_Free_NotFind;
		});

	// ��ʧ�ܵ�Task ת���������ȼ��������
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
	// �������������Ӧ�ú�ExecuteTasks���ã�
	// ����������������Ļ��Ტ���࣬����Ӱ��Ҳ����
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

			// �������δ֪���󣬲����ܣ�ֱ����������
			assert(result != BuddyTask::State::Failed_Unknown);

			// ����ɹ�ʱ����ѭ��
			if (result == BuddyTask::State::Success)
			{
				oTaskMemData.pAllocator = pAllocator;
				return result;
			}
		}
	}

	// ����ߵ����˵�����е�Allocator�����ˣ�����һ���µ�Allocator
	BuddyAllocatorPage* pNewAllocator = AddAllocatorInternal();
	result = pNewAllocator->AllocSync(task, oTaskMemData.byteOffset);
	if (result == BuddyTask::State::Success)
	{
		oTaskMemData.pAllocator = pNewAllocator;
		return result;
	}

	// �������δ֪���󣬲����ܣ�ֱ����������
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
	// ������ȷ��byteSize��Ӧ����С�ڴ�鼶��
	uint32_t newBlockLevel = m_pOwner->GetLevel(task.byteSize);

	bool hasMinBlock = false;
	uint32_t minBlockByteOffset;
	BuddyMemoryBlock pMinBlock;

	// ����ڴ�飬����Ѿ���ͬ���ڴ�飬��ֱ��ʹ��
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

			// ���û��ͬ���ڴ�飬�ҵ�һ���ȶ������󣬵���С���ڴ�飨�ȼ�ԽС���ڴ��Խ��
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

	// ����ҵ��˴��ڶ�������С�ڴ�飬����ָ�ɶ������ڴ��
	if (hasMinBlock)
	{
		m_memory.erase(minBlockByteOffset);

		// for �𼶲��
		for (uint32_t i = pMinBlock.level; i < newBlockLevel; i++)
		{
			// ��Զֻ�����ӿ��ֳ�ȥ
			BuddyMemoryBlock halfBlock;
			halfBlock.level = i + 1;
			halfBlock.bUsed = false;

			uint32_t halfByteOffset = minBlockByteOffset + m_pOwner->GetByteSizeFromLevel(i + 1);
			m_memory.insert({ halfByteOffset, halfBlock });

			// ��ֵ�����ʱ�������ӿ���Ϊ��ʹ��
			if (i + 1 == newBlockLevel)
			{
				halfBlock.bUsed = true;
				oByteOffset = minBlockByteOffset;
				m_memory.insert({ oByteOffset , halfBlock });

				// ����ɹ������Һ���û���߼�������ֱ�ӷ�����
				return BuddyTask::State::Success; 
			}
		}
	}
	
	// �Ҳ���˵���ڴ�����
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

	// �𼶺ϲ�
	for (uint32_t i = it->second.level; i > 0; i--)
	{
		// ��ǰ�ڴ����ֽڴ�С
		uint32_t freeByteSize = m_pOwner->GetByteSizeFromLevel(i);

		// xor �������ڴ�ƫ����
		uint32_t buddyByteOffset = freeByteOffset ^ freeByteSize;

		uint32_t parentByteOffset = std::min(freeByteOffset, buddyByteOffset);

		auto it = m_memory.find(buddyByteOffset);
		if (it == m_memory.end())
			throw std::runtime_error("buddy not found."); // ֱ�����쳣����Ӧ���Ҳ���buddy
		
		if (it->second.bUsed == false)
		{
			// ʵ�ʵĺϲ���Ϊ
			m_memory.erase(freeByteOffset);
			m_memory.erase(buddyByteOffset);

			BuddyMemoryBlock block;
			block.level = i - 1;
			block.bUsed = false;
			m_memory.insert({ parentByteOffset, block });
		}
		else
		{
			// �ߵ�����˵������ڴ�鱻ռ�ã����ټ������Ϻϲ�
			break;
		}
	}

	return BuddyTask::State::Success;
}

void ccmem::BuddyAllocatorPage::Print()
{
}
