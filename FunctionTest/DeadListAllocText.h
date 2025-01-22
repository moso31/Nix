#pragma once
#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <cassert>
#include <iostream>

namespace ccmem
{
	struct DeadListTaskResult
	{
		uint8_t* pMemory;
	};

	struct DeadListTask
	{
		std::function<void(DeadListTaskResult&)> pCallback;
		uint8_t* pFreeMem;
	};

	struct DeadListBlock
	{
		uint8_t* pMem;
	};

	class DeadListAllocator
	{
	public:
		// 所谓deadList 其实就是一个记录了空闲内存块的链表
		// blockSize 表示内存块的数量，blockByteSize 表示每个内存块的字节大小
		DeadListAllocator(uint32_t blockByteSize, uint32_t blockSize);
		~DeadListAllocator();

		void Alloc(const std::function<void(DeadListTaskResult&)>& callback);
		void Free(uint8_t* pMem);

		void ExecuteTasks();
		void Print();

	private:
		std::vector<DeadListBlock> m_deadList;
		uint32_t m_currentDeadListIndex;

		uint32_t m_blockByteSize;
		uint8_t* m_pMem;

		std::list<DeadListTask> m_taskList;
		std::mutex m_mutex;
	};
}