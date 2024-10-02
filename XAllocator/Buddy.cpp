#include "Buddy.h"

using namespace ccmem;

ccmem::BuddyAllocator::BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize) :
	MIN_LV(blockByteSize),
	MAX_LV(fullByteSize)
{
	// ������С����󼶱��log2ֵ �� LV_NUM
	for (MIN_LV_LOG2 = 0; uint32_t(1 << MIN_LV_LOG2) < MIN_LV; ++MIN_LV_LOG2);
	for (MAX_LV_LOG2 = 0; uint32_t(1 << MAX_LV_LOG2) < MAX_LV; ++MAX_LV_LOG2);
	LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1;

	// ��ʼ��ʱ�Զ�����һ��Allocator 
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

	// �������ȼ�������кϲ�����������У����������ִ��
	taskList.splice(taskList.end(), m_lowPriorTaskList);
	m_lowPriorTaskList.clear();

	for (auto& task : taskList)
	{
		if (!task.pFreeMem) // ����
		{
			BuddyTaskResult taskResult;
			task.state = TryAlloc(task, taskResult);

			// �������ɹ��������ص�����
			if (task.state == BuddyTask::State::Success)
				task.pCallBack(taskResult);
		}
		else // �ͷ�
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
	// �������������Ӧ�ú�ExecuteTasks���ã�
	// ����������������Ļ��Ტ���࣬����Ӱ��Ҳ����
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

			// �������δ֪���󣬲����ܣ�ֱ����������
			assert(result != BuddyTask::State::Failed_Unknown);

			// ����ɹ�ʱ����ѭ��
			if (result == BuddyTask::State::Success)
			{
				oTaskResult.pAllocator = pAllocator;
				return result;
			}
		}
	}

	// ����ߵ����˵�����е�Allocator�����ˣ�����һ���µ�Allocator
	BuddyAllocatorPage* pNewAllocator = AddAllocatorInternal();
	result = pNewAllocator->AllocSync(task, oTaskResult.pMemory);
	if (result == BuddyTask::State::Success)
	{
		oTaskResult.pAllocator = pNewAllocator;
		return result;
	}

	// �������δ֪���󣬲����ܣ�ֱ����������
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
	// ȷ��byteSize��Ӧ����С�ڴ�鼶��
	int newBlockLevel = m_pOwner->GetLevel(task.byteSize);

	// �Ӹü���ʼ���ϲ��ң��ҵ����õ��ڴ��
	for (int i = newBlockLevel; i >= 0; i--)
	{
		if (!m_freeList[i].empty())
		{
			pAllocMem = AllocInternal(newBlockLevel, i);
			if (pAllocMem)
			{
				m_freeByteSize -= m_pOwner->GetAlignedByteSize(task.byteSize);
				return BuddyTask::State::Success; // ����ɹ�
			}
			else
			{
				pAllocMem = nullptr;
				return BuddyTask::State::Failed_Unknown; // ����ʧ�ܣ�Ŀǰ��ʱû���ߵ������������ȷ��ظ�δ֪�����
			}
		}
	}

	// ����ߵ����˵���ڴ��Ѿ�����
	pAllocMem = nullptr;
	return BuddyTask::State::Failed_Alloc_FullMemory;
}

BuddyTask::State ccmem::BuddyAllocatorPage::FreeSync(const BuddyTask& task)
{
	// ��used�б����ҵ����ڴ��
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

	// ����иü�����ڴ����ã�ֱ��ʹ��
	if (destLevel == srcLevel)
	{
		// ��ȡ���б�ĵ�һ��Ԫ�أ������õ��ڴ��
		uint8_t* p = m_freeList[destLevel].front();
		m_freeList[destLevel].pop_front(); // ��free�б���ɾ����Ԫ��
		m_usedList[destLevel].push_back(p); // ��used�б�����Ӹ�Ԫ��
		return p;
	}

	// ������ƾ�����srcLevel��һ���к��ʵ��ڴ�飨�����ڴ����ˣ�
	if (!m_freeList[srcLevel].empty())
	{
		// �Ӹü���Ŀ����б���ȡ��һ���ڴ��
		uint8_t* p = m_freeList[srcLevel].front();
		m_freeList[srcLevel].pop_front();

		// �����ڴ��һ��Ϊ������������һ����Ŀ����б�
		uint32_t halfBlockSize = 1 << (m_pOwner->GetMaxLevelLog2() - srcLevel - 1);
		m_freeList[srcLevel + 1].push_back(p);
		m_freeList[srcLevel + 1].push_back(p + halfBlockSize);

		// �ݹ����һ����ֱ���ҵ����ʵ��ڴ��
		return AllocInternal(destLevel, srcLevel + 1);
	}

	return nullptr;
}

void ccmem::BuddyAllocatorPage::FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level)
{
	if (level == 0) return;

	// itMem ָ��Ҫ�ͷŵ��ڴ��
	uint8_t* pMem = *itMem;

	// �����ڴ���buddy�Ƿ���free�б��У�������ھͺϲ���һ������Ŀ�
	// ʹ��XOR�����ҵ�buddy
	uint32_t blockSize = 1 << (m_pOwner->GetMaxLevelLog2() - level);
	uintptr_t offset = reinterpret_cast<uintptr_t>(pMem) - reinterpret_cast<uintptr_t>(m_pMem);
	uintptr_t buddyOffset = offset ^ blockSize;
	uint8_t* pBuddyMem = m_pMem + buddyOffset;

	// ��free�б����ҵ�buddy
	auto itBuddyMem = std::find(m_freeList[level].begin(), m_freeList[level].end(), pBuddyMem);
	if (itBuddyMem != m_freeList[level].end())
	{
		// �ϲ�
		m_freeList[level].erase(itMem);
		m_freeList[level].erase(itBuddyMem);

		uint8_t* pMergedMem = std::min(pMem, pBuddyMem);
		m_freeList[level - 1].push_back(pMergedMem);

		// �ݹ鴦�����һ�����ڴ��
		FreeInternal(std::prev(m_freeList[level - 1].end()), level - 1);
	}

}