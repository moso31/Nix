// 2024.10.4 BuddyAllocator 基础管理类 by moso31
// 描述：
//		BuddyAllocator 类是基础的内存管理器，
//		负责管理内存块的分级、任务队列的处理以及内存页（BuddyAllocatorPage）的整体调度。
// 
// 逻辑：
//		内存级别划分：根据最小和最大内存块大小，计算出不同的内存级别，每一级对应特定大小的内存块。
//		任务管理：维护一个任务队列，包括待分配和待释放的内存操作。通过单线程的 ExecuteTasks 方法来执行这些任务。
//		分页管理：当现有的内存页无法满足分配需求时，动态添加新的内存页（BuddyAllocatorPage）。
// 
// 设计理念：
//		解耦：将内存管理的通用逻辑与具体的内存分配实现分离，便于扩展和维护。
//		扩展性：允许派生类根据不同的需求，实现特定的内存分配和释放策略。

#pragma once
#include "XAllocCommon.h"

namespace ccmem
{
	class BuddyAllocatorPage;
	struct BuddyTask;

	struct XBuddyTaskMemData
	{
		BuddyAllocatorPage* pAllocator;
		uint32_t byteOffset;
	};

	struct BuddyTaskResult
	{
		BuddyTaskResult(const BuddyTask& connectTask);
		~BuddyTaskResult();

		uint64_t selfID;
		uint64_t connectTaskID;

		uint8_t* pTaskContext;
		XBuddyTaskMemData memData;
	};

	struct BuddyTask
	{
		BuddyTask();

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

		uint64_t selfID;

		// 这俩字段只会有一个被使用
		union 
		{
			uint32_t byteSize = 0;	// 要么记录Alloc的内存大小
			uint32_t offset;		// 要么记录Free 的内存偏移量
		};

		// 回调函数
		std::function<void(const BuddyTaskResult&)> pCallBack = nullptr;

		// 记录task的执行状态
		BuddyTask::State state = BuddyTask::State::Pending;

		// 上下文，用于传递一些额外的信息（比如分配PlacedResource类型时需要提供D3D12_RESOURCE_DESC）
		uint8_t* pTaskContext;
		uint32_t pTaskContextSize = 0;

		// 释放时 记录要释放的内存所属的内存页指针
		BuddyAllocatorPage* pFreeAllocator;
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
		BuddyTask::State AllocSync(const BuddyTask& task, uint32_t& oByteOffset);
		BuddyTask::State FreeSync(const uint32_t& freeByteOffset);

		bool AllocInternal(uint32_t destLevel, uint32_t srcLevel, uint32_t& oByteOffset);
		void FreeInternal(std::list<uint32_t>::iterator itMem, uint32_t level);

		uint32_t GetPageID() const { return m_pageID; }

	private:
		BuddyAllocator* m_pOwner;
		std::atomic_uint32_t m_freeByteSize;

		// 按2的幂次将 可分配的内存块大小 划分N级, 每级都用一个链表管理
		// 0 级 = 最大的内存块, N-1 级 = 最小的内存块
		std::vector<std::list<uint32_t>> m_freeList;
		std::vector<std::list<uint32_t>> m_usedList;

		uint32_t m_pageID;
	};

	class BuddyAllocator
	{
	public:
		// blockByteSize = 单个内存块的大小 fullByteSize = 总内存大小
		BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize);

		void ExecuteTasks();
		void Print();

		uint32_t GetMaxLevel() const { return MAX_LV; }
		uint32_t GetMaxLevelLog2() const { return MAX_LV_LOG2; }
		uint32_t GetMinLevel() const { return MIN_LV; }
		uint32_t GetMinLevelLog2() const { return MIN_LV_LOG2; }
		uint32_t GetLevelNum() const { return LV_NUM; }

		uint32_t GetLevel(uint32_t byteSize);
		uint32_t GetAlignedByteSize(uint32_t byteSize);

	protected:
		virtual void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) = 0;

		// 添加一个分配任务，异步执行
		// byteSize: 分配的内存大小
		// pTaskContext, pTaskContextSize: 任务上下文信息及大小
		void AddAllocTask(uint32_t byteSize, void* pTaskContext, uint32_t pTaskContextSize, const std::function<void(const BuddyTaskResult&)>& callback);
		void AddFreeTask(BuddyAllocatorPage* pAllocator, uint32_t pFreeMem);

	private:
		BuddyAllocatorPage* AddAllocatorInternal();
		BuddyTask::State TryAlloc(const BuddyTask& task, XBuddyTaskMemData& oTaskMemData);
		BuddyTask::State TryFree(const BuddyTask& task);
	private:
		std::mutex m_mutex;

		// 任务队列，任务首次添加总是添加到这里
		std::list<BuddyTask> m_taskList;

		// 低优先级任务队列，当任务执行失败时，会被移动到这里，
		std::list<BuddyTask> m_lowPriorTaskList;

		// 在这里统一管理所有的Allocator
		std::list<BuddyAllocatorPage*> m_allocatorPages;

		uint32_t MAX_LV_LOG2;
		uint32_t MAX_LV;// = 1 << MAX_LV_LOG2;
		uint32_t MIN_LV_LOG2;
		uint32_t MIN_LV;// = 1 << MIN_LV_LOG2;
		uint32_t LV_NUM;// = MAX_LV_LOG2 - MIN_LV_LOG2 + 1; // 链表的数量，一共N个
	};
}
