#pragma once
#include "header.h"

class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript();

	void SetObject(NXObject* pObject);

	virtual void Update() = 0;

protected:
	NXObject* m_pObject;
};
