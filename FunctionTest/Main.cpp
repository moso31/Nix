#include "MemAllocTest.h"
#include <vector>

using namespace ccmem;

int main()
{
	BuddyAllocatorTaskQueue queue;
	BuddyAllocator allocator(&queue);

	// �����ѷ�����ڴ����Ϣ
	struct AllocatedBlock
	{
		uint8_t* ptr;
		uint32_t size;
	};

	std::vector<AllocatedBlock> allocatedBlocks;

	std::string command;
	const uint32_t MIN_SIZE = 1; // ������С�����С
	const uint32_t MAX_SIZE = 16; // �����������С

	while (true)
	{
		std::cout << "������ָ�� (alloc/free/exit): ";
		std::cin >> command;

		if (command == "a")
		{
			uint32_t requestSize = std::rand() % (MAX_SIZE - MIN_SIZE + 1) + MIN_SIZE;
			std::cout << "Alloc: " << requestSize << " bytes." << std::endl;

			allocator.AllocAsync(requestSize, [&allocatedBlocks, requestSize](uint8_t* pMem) {
				if (pMem) allocatedBlocks.push_back({ pMem, requestSize });
				});
		}
		else if (command == "f")
		{
			if (allocatedBlocks.empty())
			{
				std::cout << "û�п��ͷŵ��ڴ�顣" << std::endl;
			}
			else
			{
				int index = std::rand() % allocatedBlocks.size();
				uint8_t* pMem = allocatedBlocks[index].ptr;
				uint32_t size = allocatedBlocks[index].size;
				std::cout << "Free: " << size << " bytes." << std::endl;

				allocator.FreeAsync(pMem);
				allocatedBlocks.erase(allocatedBlocks.begin() + index);
			}
		}
		else if (command == "e")
		{
			break; // �˳�ѭ��
		}
		else
		{
			std::cout << "��Ч��ָ������� alloc��free �� exit��" << std::endl;
		}

		// ִ����������е�����
		queue.ExecuteTasks();

		// ��ӡ�ڴ�������
		allocator.Print();
	}

	return 0;
}

