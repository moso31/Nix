#pragma once
#include <list>
#include <cassert>
#include <iostream>
#include <functional>

namespace ccmem
{
	const uint32_t MAX_LV_LOG2 = 31; // 2^31 = 2 GB
	const uint32_t MAX_LV = 1 << MAX_LV_LOG2;
	const uint32_t MIN_LV_LOG2 = 6; // 2^6 = 64 B
	const uint32_t MIN_LV = 1 << MIN_LV_LOG2;
	const uint32_t LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1; // 链表的数量，一共N个

	class BuddyAllocator;

	struct BuddyAllocatorTask
	{
		// 使用的BuddyAllocator对象
		BuddyAllocator* pAllocator;

		// 分配的字节大小，释放时不用提供
		uint32_t byteSize = 0;

		// 释放时需要提供内存；分配时不需要
		uint8_t* pMem = nullptr; 

		// 回调函数，用于获取分配的内存指针
		std::function<void(uint8_t*)> callback = nullptr;
	};

	class BuddyAllocatorTaskQueue
	{
	public:
		void Push(BuddyAllocatorTask task);

		// 从队列中取出任务并执行，定期持续调用
		void ExecuteTasks(); 

	private:
		std::list<BuddyAllocatorTask> m_taskList;
	};

	class BuddyAllocator
	{
		friend class BuddyAllocatorTaskQueue;
	public:
		BuddyAllocator(BuddyAllocatorTaskQueue* pQueue);
		~BuddyAllocator();

		void Print();

		void AllocAsync(uint32_t byteSize, std::function<void(uint8_t*)> callback);
		void FreeAsync(uint8_t* pMem);

	private:
		uint8_t* AllocSync(uint32_t byteSize);
		void FreeSync(uint8_t* pMem);

		uint8_t* AllocInternal(uint32_t destLevel, uint32_t srcLevel);
		void FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level);
		uint32_t GetLevel(uint32_t byteSize);

	private:
		BuddyAllocatorTaskQueue* m_pTaskQueue;
		uint8_t* m_pMem;

		// 按2的幂次将 可分配的内存块大小 划分N级, 每级都用一个链表管理
		// 0 级 = 最大的内存块, N-1 级 = 最小的内存块
		std::list<uint8_t*> m_freeList[LV_NUM];
		std::list<uint8_t*> m_usedList[LV_NUM];
	};
}
