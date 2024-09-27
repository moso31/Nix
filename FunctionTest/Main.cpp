#include "MemAllocTest.h"
#include <vector>
#include <string>

using namespace ccmem;

int main()
{
	BuddyAllocator taskMan;

	// 保存已分配的内存块信息
	struct AllocatedBlock
	{
		uint8_t* ptr;
		uint32_t size;
		BuddyAllocatorPage* allocator;
	};

	std::vector<AllocatedBlock> allocatedBlocks;

	std::string command, command2;
	const uint32_t MIN_SIZE = 1; // 定义最小分配大小
	const uint32_t MAX_SIZE = 16; // 定义最大分配大小

	while (true)
	{
		std::cout << "请输入指令 (alloc/free/exit): ";
		std::cin >> command;

		if (command == "a")
		{
			std::cin >> command2;
			uint32_t requestSize = std::stoi(command2); // 将用户输入的字符串转换为整数
			//uint32_t requestSize = std::rand() % (MAX_SIZE - MIN_SIZE + 1) + MIN_SIZE;
		}
		else if (command == "f")
		{
		}
		else if (command == "e")
		{
			break; // 退出循环
		}
		else
		{
			std::cout << "无效的指令，请输入 alloc、free 或 exit。" << std::endl;
		}

		// 执行任务队列中的任务
		taskMan.ExecuteTasks();

		// 打印内存分配情况
		allocator->Print();
	}

	return 0;
}

