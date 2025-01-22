#include "BuddyAllocTest.h"
#include "DeadListAllocText.h"
#include <vector>
#include <string>

using namespace ccmem;

//int main()
//{
//	BuddyAllocator taskMan;
//
//	// �����ѷ�����ڴ����Ϣ
//	struct AllocatedBlock
//	{
//		uint8_t* ptr;
//		uint32_t size;
//		BuddyAllocatorPage* allocator;
//	};
//
//	std::vector<AllocatedBlock> allocatedBlocks;
//
//	std::string command, command2;
//	const uint32_t MIN_SIZE = 1; // ������С�����С
//	const uint32_t MAX_SIZE = 16; // �����������С
//
//	while (true)
//	{
//		std::cout << "������ָ�� (alloc/free/exit): ";
//		std::cin >> command;
//
//		if (command == "a")
//		{
//			std::cin >> command2;
//			uint32_t requestSize = std::stoi(command2); // ���û�������ַ���ת��Ϊ����
//			//uint32_t requestSize = std::rand() % (MAX_SIZE - MIN_SIZE + 1) + MIN_SIZE;
//
//			taskMan.Alloc(requestSize, [&](const BuddyTaskResult& result) {
//				AllocatedBlock ab;
//				ab.allocator = result.pAllocator;
//				ab.ptr = result.pMemory;
//				ab.size = requestSize;
//
//				allocatedBlocks.push_back(ab);
//			});
//		}
//		else if (command == "f")
//		{
//			if (!allocatedBlocks.empty())
//			{
//				int releaseIndex = std::rand() % allocatedBlocks.size();
//
//				AllocatedBlock& ab = allocatedBlocks[releaseIndex];
//				taskMan.Free(ab.ptr);
//
//				allocatedBlocks.erase(allocatedBlocks.begin() + releaseIndex);
//			}
//		}
//		else if (command == "e")
//		{
//			break; // �˳�ѭ��
//		}
//		else
//		{
//			std::cout << "��Ч��ָ������� alloc��free �� exit��" << std::endl;
//		}
//
//		// ִ����������е�����
//		taskMan.ExecuteTasks();
//
//		// ��ӡ�ڴ�������
//		taskMan.Print();
//	}
//
//	return 0;
//}

int main()
{
    // ��ʼ�������������
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // ����һ��DeadListAllocator��ÿ���ڴ���СΪ256�ֽڣ��ܹ�10���ڴ��
    ccmem::DeadListAllocator allocator(256, 10);
    std::vector<uint8_t*> allocatedBlocks; // �洢�ѷ�����ڴ��ָ��

    std::string command;

    while (true)
    {
        std::cout << "������ָ�� (alloc/free/print/exit): ";
        std::cin >> command;

        if (command == "alloc" || command == "a")
        {
            // ִ��alloc����
            allocator.Alloc([&](ccmem::DeadListTaskResult& result) {
                if (result.pMemory != nullptr)
                {
                    allocatedBlocks.push_back(result.pMemory);
                    std::cout << "�ѷ����ڴ�飬��ַ: " << static_cast<void*>(result.pMemory) << std::endl;
                }
                else
                {
                    std::cout << "�ڴ����ʧ�ܣ�û�п��õ��ڴ�顣" << std::endl;
                }
                });
            allocator.ExecuteTasks();
        }
        else if (command == "free" || command == "f")
        {
            if (allocatedBlocks.empty())
            {
                std::cout << "û���ѷ�����ڴ����ͷš�" << std::endl;
                continue;
            }

            // ���ѡ��һ���ѷ�����ڴ������ͷ�
            size_t index = std::rand() % allocatedBlocks.size();
            uint8_t* pMem = allocatedBlocks[index];

            allocator.Free(pMem);
            allocator.ExecuteTasks();

            // ���ѷ����б����Ƴ�
            allocatedBlocks.erase(allocatedBlocks.begin() + index);
            std::cout << "���ͷ��ڴ�飬��ַ: " << static_cast<void*>(pMem) << std::endl;
        }
        else if (command == "print" || command == "p")
        {
            // ����ѷ���Ϳ��е��ڴ����Ϣ
            std::cout << "====================================" << std::endl;
            std::cout << "�ѷ����ڴ������: " << allocatedBlocks.size() << std::endl;
            std::cout << "�ѷ����ڴ���ַ��" << std::endl;
            for (size_t i = 0; i < allocatedBlocks.size(); ++i)
            {
                std::cout << "  [" << i << "]: " << static_cast<void*>(allocatedBlocks[i]) << std::endl;
            }
            std::cout << std::endl;

            // ����allocator��Print��������������ڴ����Ϣ
        }
        else if (command == "exit" || command == "e")
        {
            break;
        }
        else
        {
            std::cout << "��Ч��ָ������� 'alloc'��'free'��'print' �� 'exit'��" << std::endl;
        }

        allocator.Print();
    }

    return 0;
}