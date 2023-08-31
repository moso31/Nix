#pragma once
#include "NXScript.h"

class NXPrimitive;
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