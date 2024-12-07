// 2024.10.4 BuddyAllocator ���������� by moso31
// ������
//		BuddyAllocator ���ǻ������ڴ��������
//		��������ڴ��ķּ���������еĴ����Լ��ڴ�ҳ��BuddyAllocatorPage����������ȡ�
// 
// �߼���
//		�ڴ漶�𻮷֣�������С������ڴ���С���������ͬ���ڴ漶��ÿһ����Ӧ�ض���С���ڴ�顣
//		�������ά��һ��������У�����������ʹ��ͷŵ��ڴ������ͨ�����̵߳� ExecuteTasks ������ִ����Щ����
//		��ҳ���������е��ڴ�ҳ�޷������������ʱ����̬����µ��ڴ�ҳ��BuddyAllocatorPage����
// 
// ������
//		������ڴ�����ͨ���߼��������ڴ����ʵ�ַ��룬������չ��ά����
//		��չ�ԣ�������������ݲ�ͬ������ʵ���ض����ڴ������ͷŲ��ԡ�

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
			// �ȴ�ִ��
			Pending,

			// �ɹ�
			Success,

			// ʧ�ܣ�����ʱ�ڴ�����
			Failed_Alloc_FullMemory,

			// ʧ�ܣ��ͷ�ʱδ�ҵ���Ӧ�ڴ�
			Failed_Free_NotFind,

			// δ֪����
			Failed_Unknown,
		};

		uint64_t selfID;

		// �����ֶ�ֻ����һ����ʹ��
		union 
		{
			uint32_t byteSize = 0;	// Ҫô��¼Alloc���ڴ��С
			uint32_t offset;		// Ҫô��¼Free ���ڴ�ƫ����
		};

		// �ص�����
		std::function<void(const BuddyTaskResult&)> pCallBack = nullptr;

		// ��¼task��ִ��״̬
		BuddyTask::State state = BuddyTask::State::Pending;

		// �����ģ����ڴ���һЩ�������Ϣ���������PlacedResource����ʱ��Ҫ�ṩD3D12_RESOURCE_DESC��
		uint8_t* pTaskContext;
		uint32_t pTaskContextSize = 0;

		// �ͷ�ʱ ��¼Ҫ�ͷŵ��ڴ��������ڴ�ҳָ��
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

		// ��2���ݴν� �ɷ�����ڴ���С ����N��, ÿ������һ���������
		// 0 �� = �����ڴ��, N-1 �� = ��С���ڴ��
		std::vector<std::list<uint32_t>> m_freeList;
		std::vector<std::list<uint32_t>> m_usedList;

		uint32_t m_pageID;
	};

	class BuddyAllocator
	{
	public:
		// blockByteSize = �����ڴ��Ĵ�С fullByteSize = ���ڴ��С
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

		// ���һ�����������첽ִ��
		// byteSize: ������ڴ��С
		// pTaskContext, pTaskContextSize: ������������Ϣ����С
		void AddAllocTask(uint32_t byteSize, void* pTaskContext, uint32_t pTaskContextSize, const std::function<void(const BuddyTaskResult&)>& callback);
		void AddFreeTask(BuddyAllocatorPage* pAllocator, uint32_t pFreeMem);

	private:
		BuddyAllocatorPage* AddAllocatorInternal();
		BuddyTask::State TryAlloc(const BuddyTask& task, XBuddyTaskMemData& oTaskMemData);
		BuddyTask::State TryFree(const BuddyTask& task);
	private:
		std::mutex m_mutex;

		// ������У������״����������ӵ�����
		std::list<BuddyTask> m_taskList;

		// �����ȼ�������У�������ִ��ʧ��ʱ���ᱻ�ƶ������
		std::list<BuddyTask> m_lowPriorTaskList;

		// ������ͳһ�������е�Allocator
		std::list<BuddyAllocatorPage*> m_allocatorPages;

		uint32_t MAX_LV_LOG2;
		uint32_t MAX_LV;// = 1 << MAX_LV_LOG2;
		uint32_t MIN_LV_LOG2;
		uint32_t MIN_LV;// = 1 << MIN_LV_LOG2;
		uint32_t LV_NUM;// = MAX_LV_LOG2 - MIN_LV_LOG2 + 1; // �����������һ��N��
	};
}
