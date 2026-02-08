#include "BuddyAllocTest.h"
#include "DeadListAllocText.h"
#include <vector>
#include <string>

using namespace ccmem;

//int main()
//{
//	BuddyAllocator taskMan;
//
//	// 保存已分配的内存块信息
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
//	const uint32_t MIN_SIZE = 1; // 定义最小分配大小
//	const uint32_t MAX_SIZE = 16; // 定义最大分配大小
//
//	while (true)
//	{
//		std::cout << "请输入指令 (alloc/free/exit): ";
//		std::cin >> command;
//
//		if (command == "a")
//		{
//			std::cin >> command2;
//			uint32_t requestSize = std::stoi(command2); // 将用户输入的字符串转换为整数
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
//			break; // 退出循环
//		}
//		else
//		{
//			std::cout << "无效的指令，请输入 alloc、free 或 exit。" << std::endl;
//		}
//
//		// 执行任务队列中的任务
//		taskMan.ExecuteTasks();
//
//		// 打印内存分配情况
//		taskMan.Print();
//	}
//
//	return 0;
//}

int main()
{
    // 初始化随机数生成器
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // 创建一个DeadListAllocator，每个内存块大小为256字节，总共10个内存块
    ccmem::DeadListAllocator allocator(256, 10);
    std::vector<uint8_t*> allocatedBlocks; // 存储已分配的内存块指针

    std::string command;

    while (true)
    {
        std::cout << "请输入指令 (alloc/free/print/exit): ";
        std::cin >> command;

        if (command == "alloc" || command == "a")
        {
            // 执行alloc操作
            allocator.Alloc([&](ccmem::DeadListTaskResult& result) {
                if (result.pMemory != nullptr)
                {
                    allocatedBlocks.push_back(result.pMemory);
                    std::cout << "已分配内存块，地址: " << static_cast<void*>(result.pMemory) << std::endl;
                }
                else
                {
                    std::cout << "内存分配失败，没有可用的内存块。" << std::endl;
                }
                });
            allocator.ExecuteTasks();
        }
        else if (command == "free" || command == "f")
        {
            if (allocatedBlocks.empty())
            {
                std::cout << "没有已分配的内存块可释放。" << std::endl;
                continue;
            }

            // 随机选择一个已分配的内存块进行释放
            size_t index = std::rand() % allocatedBlocks.size();
            uint8_t* pMem = allocatedBlocks[index];

            allocator.Free(pMem);
            allocator.ExecuteTasks();

            // 从已分配列表中移除
            allocatedBlocks.erase(allocatedBlocks.begin() + index);
            std::cout << "已释放内存块，地址: " << static_cast<void*>(pMem) << std::endl;
        }
        else if (command == "print" || command == "p")
        {
            // 输出已分配和空闲的内存块信息
            std::cout << "====================================" << std::endl;
            std::cout << "已分配内存块数量: " << allocatedBlocks.size() << std::endl;
            std::cout << "已分配内存块地址：" << std::endl;
            for (size_t i = 0; i < allocatedBlocks.size(); ++i)
            {
                std::cout << "  [" << i << "]: " << static_cast<void*>(allocatedBlocks[i]) << std::endl;
            }
            std::cout << std::endl;

            // 调用allocator的Print函数，输出空闲内存块信息
        }
        else if (command == "exit" || command == "e")
        {
            break;
        }
        else
        {
            std::cout << "无效的指令，请输入 'alloc'、'free'、'print' 或 'exit'。" << std::endl;
        }

        allocator.Print();
    }

    return 0;
}