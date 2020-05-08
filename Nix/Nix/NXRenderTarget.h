#pragma once
#include "NXPrimitive.h"

class NXRenderTarget : public NXPrimitive
{
public:
	NXRenderTarget();
	~NXRenderTarget() {}

	void Init();
	void Render();

private:
};

