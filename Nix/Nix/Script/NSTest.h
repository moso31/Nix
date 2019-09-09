#pragma once
#include "NXScript.h"

class NSTest : public NXScript
{
public:
	NSTest();

	void Update();

private:
	shared_ptr<NXPrimitive> pPrimitive;
	float m_rotValue;
};