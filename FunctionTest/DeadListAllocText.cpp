#include "DeadListAllocText.h"

ccmem::DeadListAllocator::DeadListAllocator(uint32_t blockByteSize, uint32_t blockSize) :
	m_blockByteSize(blockByteSize)
{
	m_pMem = new uint8_t[blockByteSize * blockSize];

	m_deadList.reserve(blockSize);
	for (size_t i = 0; i < blockSize; i++)
	{
		DeadListBlock block;
		block.pMem = m_pMem + (blockByteSize * i);
		m_deadList.push_back(block);
	}

	m_currentDeadListIndex = 0;
}

ccmem::DeadListAllocator::~DeadListAllocator()
{
	delete[] m_pMem;
}

void ccmem::DeadListAllocator::Alloc(const std::function<void(DeadListTaskResult&)>& callback)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	DeadListTask task;
	task.pCallback = callback;
	task.pFreeMem = nullptr;
	m_taskList.push_back(task);
}

void ccmem::DeadListAllocator::Free(uint8_t* pMem)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	DeadListTask task;
	task.pCallback = nullptr;
	task.pFreeMem = pMem;
	m_taskList.push_back(task);
}

void ccmem::DeadListAllocator::ExecuteTasks()
{
	m_mutex.lock();
	std::list<DeadListTask> taskList;
	taskList.swap(m_taskList);
	m_mutex.unlock();

	for (auto& task: taskList)
	{
		// Alloc
		if (task.pFreeMem == nullptr)
		{
			if (m_currentDeadListIndex >= m_deadList.size())
			{
				// No more memory. ���ڴ��ڳ��ڿ��� �Ȳ��������������assert���������
				assert(false);
				break;
			}

			DeadListTaskResult result;
			result.pMemory = m_deadList[m_currentDeadListIndex].pMem;
			m_currentDeadListIndex++;
			task.pCallback(result);
		}
		else // Free
		{
			m_currentDeadListIndex--;
			m_deadList[m_currentDeadListIndex].pMem = task.pFreeMem;
		}
	}

	taskList.clear();
}

void ccmem::DeadListAllocator::Print()
{
	std::cout << "��ǰDeadListAllocator״̬��" << std::endl;
	std::cout << "���ڴ������: " << m_deadList.size() << std::endl;
	std::cout << "�ѷ����ڴ������: " << m_currentDeadListIndex << std::endl;
	std::cout << "�����ڴ������: " << m_deadList.size() - m_currentDeadListIndex << std::endl;
	//std::cout << "δ�����ڴ���ַ��" << std::endl;
	//for (size_t i = m_currentDeadListIndex; i < m_deadList.size(); i++)
	//{
	//	std::cout << "  [" << i << "]: " << static_cast<void*>(m_deadList[i].pMem) << std::endl;
	//}
	std::cout << std::endl;
}