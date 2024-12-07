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

void ccmem::BuddyAllocator::AddAllocTask(uint32_t byteSize, void* pTaskContext, uint32_t pTaskContextSize, const std::function<void(const BuddyTaskResult&)>& callback)
{
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

	m_freeList.resize(pOwner->GetLevelNum());
	m_freeList[0].push_back(0);

	m_usedList.resize(pOwner->GetLevelNum());
}

ccmem::BuddyAllocatorPage::~BuddyAllocatorPage()
{
	for (int i = 0; i < m_freeList.size(); i++) m_freeList[i].clear();
	for (int i = 0; i < m_usedList.size(); i++) m_usedList[i].clear();
}

BuddyTask::State ccmem::BuddyAllocatorPage::AllocSync(const BuddyTask& task, uint32_t& oByteOffset)
{
	// ȷ��byteSize��Ӧ����С�ڴ�鼶��
	int newBlockLevel = m_pOwner->GetLevel(task.byteSize);

	// �Ӹü���ʼ���ϲ��ң��ҵ����õ��ڴ��
	for (int i = newBlockLevel; i >= 0; i--)
	{
		if (!m_freeList[i].empty())
		{
			bool bAlloc = AllocInternal(newBlockLevel, i, oByteOffset);
			if (bAlloc)
			{
				m_freeByteSize -= m_pOwner->GetAlignedByteSize(task.byteSize);
				return BuddyTask::State::Success; // ����ɹ�
			}
			else
			{
				return BuddyTask::State::Failed_Unknown; // ����ʧ�ܣ�Ŀǰ��ʱû���ߵ������������ȷ��ظ�δ֪�����
			}
		}
	}

	// ����ߵ����˵���ڴ��Ѿ�����
	return BuddyTask::State::Failed_Alloc_FullMemory;
}

BuddyTask::State ccmem::BuddyAllocatorPage::FreeSync(const uint32_t& freeByteOffset)
{
	// ��used�б����ҵ����ڴ��
	for (uint32_t i = 0; i < m_pOwner->GetLevelNum(); i++)
	{
		auto it = std::find(m_usedList[i].begin(), m_usedList[i].end(), freeByteOffset);
		if (it != m_usedList[i].end())
		{
			m_usedList[i].erase(it);
			m_freeList[i].push_back(freeByteOffset);
			FreeInternal(std::prev(m_freeList[i].end()), i);

			m_freeByteSize += 1 << (m_pOwner->GetMaxLevelLog2() - i);
			return BuddyTask::State::Success;
		}
	}

	// ����ߵ����˵�����ڴ�鲻��used�б���
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
			std::cout << freeBlock << " ";
		}

		std::cout << std::endl;
	};

	for (uint32_t i = 0; i < m_pOwner->GetLevelNum(); i++)
	{
		uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - i);
		std::cout << "  used " << i << "(size=" << blockSize << "): ";

		for (auto& usedBlock : m_usedList[i])
		{
			std::cout << usedBlock << " ";
		}

		std::cout << std::endl;
	};
}

bool ccmem::BuddyAllocatorPage::AllocInternal(uint32_t destLevel, uint32_t srcLevel, uint32_t& oByteOffset)
{
	assert(srcLevel < m_pOwner->GetLevelNum());

	// ����иü�����ڴ����ã�ֱ��ʹ��
	if (destLevel == srcLevel)
	{
		// ��ȡ���б�ĵ�һ��Ԫ�أ������õ��ڴ��
		oByteOffset = m_freeList[destLevel].front();
		m_freeList[destLevel].pop_front(); // ��free�б���ɾ����Ԫ��
		m_usedList[destLevel].push_back(oByteOffset); // ��used�б�����Ӹ�Ԫ��
		return true;
	}

	// ������ƾ�����srcLevel��һ���к��ʵ��ڴ�飨�����ڴ����ˣ�
	if (!m_freeList[srcLevel].empty())
	{
		// �Ӹü���Ŀ����б���ȡ��һ���ڴ��
		uint32_t p = m_freeList[srcLevel].front();
		m_freeList[srcLevel].pop_front();

		// �����ڴ��һ��Ϊ������������һ����Ŀ����б�
		uint32_t halfBlockSize = 1 << (m_pOwner->GetMaxLevelLog2() - srcLevel - 1);
		m_freeList[srcLevel + 1].push_back(p);
		m_freeList[srcLevel + 1].push_back(p + halfBlockSize);

		// �ݹ����һ����ֱ���ҵ����ʵ��ڴ��
		return AllocInternal(destLevel, srcLevel + 1, oByteOffset);
	}

	return false;
}

void ccmem::BuddyAllocatorPage::FreeInternal(std::list<uint32_t>::iterator itMem, uint32_t level)
{
	if (level == 0) return;

	uint32_t pByteOffset = *itMem; // itMem���ڴ�ƫ����

	// �����ڴ���buddy�Ƿ���free�б��У�������ھͺϲ���һ������Ŀ�
	// ʹ��XOR�����ҵ�buddy
	uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - level);
	uint32_t pBuddyByteOffset = pByteOffset ^ blockSize; // buddy���ڴ�ƫ����

	// ��free�б����ҵ�buddy
	auto itBuddyMem = std::find(m_freeList[level].begin(), m_freeList[level].end(), pBuddyByteOffset);
	if (itBuddyMem != m_freeList[level].end())
	{
		// �ϲ�
		m_freeList[level].erase(itMem);
		m_freeList[level].erase(itBuddyMem);

		uint32_t pMergedMem = std::min(pByteOffset, pBuddyByteOffset);
		m_freeList[level - 1].push_back(pMergedMem);

		// �ݹ鴦�����һ�����ڴ��
		FreeInternal(std::prev(m_freeList[level - 1].end()), level - 1);
	}

}
