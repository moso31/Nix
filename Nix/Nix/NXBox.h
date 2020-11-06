#pragma once
#include "NXPrimitive.h"

class NXBox : public NXPrimitive
{
public:
	NXBox();
	~NXBox() {}

	void Init(float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void Release() override;

private:
};
