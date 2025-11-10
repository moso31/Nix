#include "NXRGPassNode.h"
#include "NXResource.h"

void NXRGFrameResources::Register(NXRGHandle handle, const Ntr<NXResource>& pResource)
{
	resources[handle] = pResource;
}

const Ntr<NXResource>& NXRGFrameResources::GetRes(NXRGHandle handle) const
{
	auto it = resources.find(handle);
	if (it != resources.end())
	{
		return it->second;
	}
	
	assert(false);
	return nullptr;
}
