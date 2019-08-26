#pragma once
#include "header.h"

class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript() {}

	virtual void Update() = 0;
};