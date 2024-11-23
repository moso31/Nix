#pragma once
#include "XAllocCommon.h"

namespace ccmem
{
	struct DeadListTask;
	struct DeadListTaskResult
	{
		DeadListTaskResult(const DeadListTask& connectTask);

		uint64_t selfID;
		uint64_t connectTaskID;

		uint32_t index;
	};

	struct DeadListTask
	{
		DeadListTask();

		uint64_t selfID;

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
		void Alloc(const std::function<void(const DeadListTaskResult&)>& callback);
		void Free(uint32_t index);

	private:
		std::vector<uint32_t> m_deadList;
		uint32_t m_currentDeadListIndex;

		std::list<DeadListTask> m_taskList;
		std::mutex m_mutex;
	};
}