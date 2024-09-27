#include "MemAllocTest.h"
#include <vector>
#include <string>

using namespace ccmem;

int main()
{
	BuddyAllocator taskMan;

	// �����ѷ�����ڴ����Ϣ
	struct AllocatedBlock
	{
		uint8_t* ptr;
		uint32_t size;
		BuddyAllocatorPage* allocator;
	};

	std::vector<AllocatedBlock> allocatedBlocks;

	std::string command, command2;
	const uint32_t MIN_SIZE = 1; // ������С�����С
	const uint32_t MAX_SIZE = 16; // �����������С

	while (true)
	{
		std::cout << "������ָ�� (alloc/free/exit): ";
		std::cin >> command;

		if (command == "a")
		{
			std::cin >> command2;
			uint32_t requestSize = std::stoi(command2); // ���û�������ַ���ת��Ϊ����
			//uint32_t requestSize = std::rand() % (MAX_SIZE - MIN_SIZE + 1) + MIN_SIZE;
		}
		else if (command == "f")
		{
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
		taskMan.ExecuteTasks();

		// ��ӡ�ڴ�������
		allocator->Print();
	}

	return 0;
}

