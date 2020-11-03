#pragma once
#include "NXScript.h"

class NSTest : public NXScript
{
public:
	NSTest();
	~NSTest();

	void Update();

private:
	NXPrimitive* pPrimitive;
	float m_rotValue;
};