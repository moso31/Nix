#pragma once
#include "BaseDefs/NixCore.h"

class NXRGResourcePack
{
public:
	NXRGResourcePack();
	~NXRGResourcePack();

	void AddResource(NXRGResource* resource);
	void RemoveResource(NXRGResource* resource);
	void Clear();

	Ntr<NXRGResource> GetResource(const NXRGDescription& description);
};
