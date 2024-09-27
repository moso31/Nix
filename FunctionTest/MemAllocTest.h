/*
* 2024.9.23 Buddy 内存分配器
* 一个朴素的内存池实现，使用Buddy算法管理内存
* 
* 原理：
*	一上来就分配一个最大的内存块，然后根据需求分割成2的幂次方大小的内存块
*	同时维护一个freeList和一个usedList，分别记录**空闲内存块**和**已分配内存块**。
*	初始状态时，只有freeList中有一个最大的内存块
* 
*	分配内存时，根据ByteSize，找到对应的**level**。查找freeList[level]下是否有空闲内存块：
		如果有，直接分配
		如果没有，向上查找，直到找到一个有空闲内存块的level，
			然后将其分割成两个更小的内存块，一个用于本次分配，一个放入freeList
*	
*	释放内存时，根据内存块的大小，找到对应的level，将其放入freeList
*		然后XOR检查是否有相邻的空闲内存块，如果有，合并成一个更大的内存块，放入freeList，
*		并继续向上递归，直到没有相邻的空闲内存块
*/

#pragma once
#include <list>
#include <mutex>
#include <atomic>
#include <cassert>
#include <iostream>
#include <functional>
#include <algorithm>

namespace ccmem
{
	const uint32_t MAX_LV_LOG2 = 6; // 2^31 = 2 GB
	const uint32_t MAX_LV = 1 << MAX_LV_LOG2;
	const uint32_t MIN_LV_LOG2 = 4; // 2^6 = 64 B
	const uint32_t MIN_LV = 1 << MIN_LV_LOG2;
	const uint32_t LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1; // 链表的数量，一共N个

	class BuddyAllocatorPage;

	struct BuddyTask
	{
		enum class State
		{
			// 等待执行
			Pending,

			// 成功
			Success,

			// 失败：分配时内存已满
			Failed_Alloc_FullMemory,

			// 失败：释放时未找到对应内存
			Failed_Free_NotFind,

			// 未知错误
			Failed_Unknown,
		};

		// 记录要释放的内存地址
		uint8_t* pFreeMem = nullptr; 

		// 记录要分配的内存大小
		uint32_t byteSize = 0;

		// 记录task的执行状态
		BuddyTask::State state = BuddyTask::State::Pending;
	};

	class BuddyAllocator;
	class BuddyAllocatorPage
	{
		friend class BuddyAllocator;
	public:
		BuddyAllocatorPage(BuddyAllocator* pOwner);
		~BuddyAllocatorPage();

		void Print();

	private:
		BuddyTask::State AllocSync(const BuddyTask& task, uint8_t*& pMem);
		BuddyTask::State FreeSync(const BuddyTask& task);

		uint8_t* AllocInternal(uint32_t destLevel, uint32_t srcLevel);
		void FreeInternal(std::list<uint8_t*>::iterator itMem, uint32_t level);

		uint32_t GetLevel(uint32_t byteSize);
		uint32_t GetAlignedByteSize(uint32_t byteSize);

	private:
		BuddyAllocator* m_pOwner;

		uint8_t* m_pMem;
		std::atomic_uint32_t m_freeByteSize;

		// 按2的幂次将 可分配的内存块大小 划分N级, 每级都用一个链表管理
		// 0 级 = 最大的内存块, N-1 级 = 最小的内存块
		std::list<uint8_t*> m_freeList[LV_NUM];
		std::list<uint8_t*> m_usedList[LV_NUM];
	};

	// 只使用单个BuddyAllocator，无法处理内存分配满的状况；
	// 所以需要一个Manager，
	class BuddyAllocator
	{
	public:
		BuddyAllocator();

		void ExecuteTasks();
		void Print();

	private:
		BuddyAllocatorPage* AddAllocatorInternal();

	private:
		std::mutex m_mutex;

		BuddyTask::State TryAlloc(const BuddyTask& task, uint8_t*& pAllocMemory);
		BuddyTask::State TryFree(const BuddyTask& task);

		// 任务队列，任务首次添加总是添加到这里
		std::list<BuddyTask> m_taskList;

		// 低优先级任务队列，当任务执行失败时，会被移动到这里，
		std::list<BuddyTask> m_lowPriorTaskList;

		// 在这里统一管理所有的Allocator
		std::list<BuddyAllocatorPage*> m_allocatorPages;
	};
}
