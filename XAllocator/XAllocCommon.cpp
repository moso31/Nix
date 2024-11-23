#include "XAllocCommon.h"

uint64_t ccmem::GenerateUniqueTaskID()
{
	static std::atomic<uint64_t> s_uniqueTaskID(0);
	return s_uniqueTaskID.fetch_add(1);
}
