#include "DeadList.h"

ccmem::DeadListTaskResult::DeadListTaskResult(const DeadListTask& connectTask)
{
	selfID = ccmem::GenerateUniqueTaskID();
	connectTaskID = connectTask.selfID;

	NXPrint::Write("deadlist+ selfID: %lld, connectTaskID: %lld\n", selfID, connectTaskID);
}

ccmem::DeadListTask::DeadListTask()
{
	selfID = ccmem::GenerateUniqueTaskID();
	NXPrint::Write("deadlist+ Task ID: %lld\n", selfID);
}

ccmem::DeadListAllocator::DeadListAllocator(uint32_t size) 
{
	m_deadList.reserve(size);
	for (uint32_t i = 0; i < size; i++) m_deadList.push_back(i);
	m_currentDeadListIndex = 0;
}

ccmem::DeadListAllocator::~DeadListAllocator()
{
}

void ccmem::DeadListAllocator::Alloc(const std::function<void(const DeadListTaskResult&)>& callback)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	DeadListTask task;
	task.pCallback = callback;
	task.freeIndex = -1;
	m_taskList.push_back(task);
}

void ccmem::DeadListAllocator::Free(uint32_t index)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	DeadListTask task;
	task.pCallback = nullptr;
	task.freeIndex = index;
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
		if (task.pCallback != nullptr)
		{
			if (m_currentDeadListIndex >= m_deadList.size())
			{
				// No more memory. ���ڴ��ڳ��ڿ��� �Ȳ��������������assert���������
				assert(false);
				break;
			}

			DeadListTaskResult result(task);
			result.index = m_deadList[m_currentDeadListIndex];

			m_currentDeadListIndex++;
			task.pCallback(result);
		}
		else // Free
		{
			m_currentDeadListIndex--;
			m_deadList[m_currentDeadListIndex] = task.freeIndex;
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
