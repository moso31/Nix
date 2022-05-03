#pragma once
#include "NXRenderableObject.h"

class NXPrefab : public NXRenderableObject
{
public:
	NXPrefab();
	~NXPrefab() {}

	virtual NXPrefab* IsPrefab() override { return this; }

protected:
};