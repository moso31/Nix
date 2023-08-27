#pragma once
#include "header.h"

class NXScriptable;
class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript();

	void SetObject(NXScriptable* pObject);

	virtual void Update() = 0;

protected:
	NXScriptable* m_pObject;
};
