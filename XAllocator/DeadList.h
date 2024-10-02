#pragma once
#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <cassert>
#include <iostream>

namespace ccmem
{
	typedef uint32_t DeadListTaskResult;

	struct DeadListTask
	{
		std::function<void(DeadListTaskResult&)> pCallback;
		uint32_t freeIndex;
	};

	class DeadListAllocator
	{
	public:
		// ��νdeadList ��ʵ����һ����¼�˿����ڴ�������
		// size ��ʾlist�Ĵ�С
		DeadListAllocator(uint32_t size);
		~DeadListAllocator();

		void ExecuteTasks();
		void Print();

	protected:
		void Alloc(const std::function<void(DeadListTaskResult&)>& callback);
		void Free(uint32_t index);

	private:
		std::vector<uint32_t> m_deadList;
		uint32_t m_currentDeadListIndex;

		std::list<DeadListTask> m_taskList;
		std::mutex m_mutex;
	};
}