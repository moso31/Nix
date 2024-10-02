#include "DeadList.h"

ccmem::DeadListAllocator::DeadListAllocator(uint32_t size) 
{
	m_deadList.reserve(size);
	for (size_t i = 0; i < size; i++) m_deadList.push_back(i);
	m_currentDeadListIndex = 0;
}

ccmem::DeadListAllocator::~DeadListAllocator()
{
}

void ccmem::DeadListAllocator::Alloc(const std::function<void(DeadListTaskResult&)>& callback)
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
				// No more memory. 现在处于初期开发 先不处理这种情况，assert报个错就行
				assert(false);
				break;
			}

			DeadListTaskResult result = m_deadList[m_currentDeadListIndex];
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
	std::cout << "当前DeadListAllocator状态：" << std::endl;
	std::cout << "总内存块数量: " << m_deadList.size() << std::endl;
	std::cout << "已分配内存块数量: " << m_currentDeadListIndex << std::endl;
	std::cout << "空闲内存块数量: " << m_deadList.size() - m_currentDeadListIndex << std::endl;
	//std::cout << "未分配内存块地址：" << std::endl;
	//for (size_t i = m_currentDeadListIndex; i < m_deadList.size(); i++)
	//{
	//	std::cout << "  [" << i << "]: " << static_cast<void*>(m_deadList[i].pMem) << std::endl;
	//}
	std::cout << std::endl;
}