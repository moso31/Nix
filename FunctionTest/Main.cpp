#include "MemAllocTest.h"
#include <vector>

using namespace ccmem;

int main()
{
	BuddyAllocatorTaskQueue queue;
	BuddyAllocator allocator(&queue);

	// 保存已分配的内存块信息
	struct AllocatedBlock
	{
		uint8_t* ptr;
		uint32_t size;
	};

	std::vector<AllocatedBlock> allocatedBlocks;

	std::string command;
	const uint32_t MIN_SIZE = 1; // 定义最小分配大小
	const uint32_t MAX_SIZE = 16; // 定义最大分配大小

	while (true)
	{
		std::cout << "请输入指令 (alloc/free/exit): ";
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
				std::cout << "没有可释放的内存块。" << std::endl;
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
			break; // 退出循环
		}
		else
		{
			std::cout << "无效的指令，请输入 alloc、free 或 exit。" << std::endl;
		}

		// 执行任务队列中的任务
		queue.ExecuteTasks();

		// 打印内存分配情况
		allocator.Print();
	}

	return 0;
}

