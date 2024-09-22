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
	const uint32_t LV_NUM = MAX_LV_LOG2 - MIN_LV_LOG2 + 1; // �����������һ��N��

	class BuddyAllocator;

	struct BuddyAllocatorTask
	{
		// ʹ�õ�BuddyAllocator����
		BuddyAllocator* pAllocator;

		// ������ֽڴ�С���ͷ�ʱ�����ṩ
		uint32_t byteSize = 0;

		// �ͷ�ʱ��Ҫ�ṩ�ڴ棻����ʱ����Ҫ
		uint8_t* pMem = nullptr; 

		// �ص����������ڻ�ȡ������ڴ�ָ��
		std::function<void(uint8_t*)> callback = nullptr;
	};

	class BuddyAllocatorTaskQueue
	{
	public:
		void Push(BuddyAllocatorTask task);

		// �Ӷ�����ȡ������ִ�У����ڳ�������
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

		// ��2���ݴν� �ɷ�����ڴ���С ����N��, ÿ������һ���������
		// 0 �� = �����ڴ��, N-1 �� = ��С���ڴ��
		std::list<uint8_t*> m_freeList[LV_NUM];
		std::list<uint8_t*> m_usedList[LV_NUM];
	};
}
