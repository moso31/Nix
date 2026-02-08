#include "NXRGPassNode.h"
#include "NXResource.h"

void NXRGFrameResources::Register(NXRGHandle handle, const Ntr<NXResource>& pResource)
{
	resources[handle] = pResource;
}

Ntr<NXResource> NXRGFrameResources::GetRes(NXRGHandle handle) const
{
	// 在FrameResource里查找handle对于的实际NXResource*指针
	// 必然查的到，查不到说明代码有问题
	auto it = resources.find(handle);
	if (it != resources.end()) return it->second;
	
	assert(false);
	return nullptr;
}
