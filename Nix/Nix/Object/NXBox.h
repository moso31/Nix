#pragma once
#include "NXPrimitive.h"

class NXBox : public NXPrimitive
{
public:
	NXBox() = default;

	void Init(float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void Render();
	void Release();

private:
};
