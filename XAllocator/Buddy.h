/*
* 2024.9.23 Buddy �ڴ������
* һ�����ص��ڴ��ʵ�֣�ʹ��Buddy�㷨�����ڴ�
* 
* ԭ��
*	һ�����ͷ���һ�������ڴ�飬Ȼ���������ָ��2���ݴη���С���ڴ��
*	ͬʱά��һ��freeList��һ��usedList���ֱ��¼**�����ڴ��**��**�ѷ����ڴ��**��
*	��ʼ״̬ʱ��ֻ��freeList����һ�������ڴ��
* 
*	�����ڴ�ʱ������ByteSize���ҵ���Ӧ��**level**������freeList[level]���Ƿ��п����ڴ�飺
		����У�ֱ�ӷ���
		���û�У����ϲ��ң�ֱ���ҵ�һ���п����ڴ���level��
			Ȼ����ָ��������С���ڴ�飬һ�����ڱ��η��䣬һ������freeList
*	
*	�ͷ��ڴ�ʱ�������ڴ��Ĵ�С���ҵ���Ӧ��level���������freeList
*		Ȼ��XOR����Ƿ������ڵĿ����ڴ�飬����У��ϲ���һ��������ڴ�飬����freeList��
*		���������ϵݹ飬ֱ��û�����ڵĿ����ڴ��
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
	class BuddyAllocatorPage;

	struct BuddyTaskResult
	{
		BuddyAllocatorPage* pAllocator;
		uint8_t* pMemory;
	};

	struct BuddyTask
	{
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

		// ��¼Ҫ�ͷŵ��ڴ��ַ
		uint8_t* pFreeMem = nullptr; 

		// ��¼Ҫ������ڴ��С
		uint32_t byteSize = 0;

		// �ص�����
		std::function<void(const BuddyTaskResult&)> pCallBack = nullptr;

		// ��¼task��ִ��״̬
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

		uint32_t GetPageID() const { return m_pageID; }

	private:
		BuddyAllocator* m_pOwner;

		// ������� ֻ��������ڴ���Ҫ�ֶ�����
		uint8_t* m_pMem;
		std::atomic_uint32_t m_freeByteSize;

		// ��2���ݴν� �ɷ�����ڴ���С ����N��, ÿ������һ���������
		// 0 �� = �����ڴ��, N-1 �� = ��С���ڴ��
		std::vector<std::list<uint8_t*>> m_freeList;
		std::vector<std::list<uint8_t*>> m_usedList;

		uint32_t m_pageID;
	};

	class BuddyAllocator
	{
	public:
		// blockByteSize = �����ڴ��Ĵ�С fullByteSize = ���ڴ��С
		BuddyAllocator(uint32_t blockByteSize, uint32_t fullByteSize);

		void Alloc(uint32_t byteSize, const std::function<void(const BuddyTaskResult&)>& callback);
		void Free(uint8_t* pFreeMem);

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
		virtual void OnAllocatorAdded() = 0;

	private:
		BuddyAllocatorPage* AddAllocatorInternal();

	private:
		std::mutex m_mutex;

		BuddyTask::State TryAlloc(const BuddyTask& task, BuddyTaskResult& oTaskResult);
		BuddyTask::State TryFree(const BuddyTask& task);

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
