#pragma once
#include "NXGlobalDefinitions.h"

enum NXKeyCode
{
	LeftShift = 16,
	LeftControl = 17,
	LeftAlt = 18,
};

class NXScriptable;
class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript();

	void SetObject(NXScriptable* pObject);

	virtual void Update() = 0;
	virtual void Release();

protected:
	NXScriptable* m_pObject;
};
