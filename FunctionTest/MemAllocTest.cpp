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
	// ȷ��byteSize��Ӧ����С�ڴ�鼶��
	int newBlockLevel = GetLevel(byteSize);

	// �Ӹü���ʼ���ϲ��ң��ҵ����õ��ڴ��
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
	// ��used�б����ҵ����ڴ��
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
		uint32_t halfBlockSize = 1 << (MAX_LV_LOG2 - srcLevel - 1);
		m_freeList[srcLevel + 1].push_back(p);
		m_freeList[srcLevel + 1].push_back(p + halfBlockSize);

		// �ݹ����һ����ֱ���ҵ����ʵ��ڴ��
		return AllocInternal(destLevel, srcLevel + 1);
	}

	return nullptr;
}

void ccmem::BuddyAllocator::FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level)
{
	// itMem ָ��Ҫ�ͷŵ��ڴ��
	uint8_t* pMem = *itMem;

	// �����ڴ���buddy�Ƿ���free�б��У�������ھͺϲ���һ������Ŀ�
	// ʹ��XOR�����ҵ�buddy
	uint32_t blockSize = 1 << (MAX_LV_LOG2 - level);
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
