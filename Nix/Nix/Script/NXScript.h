#pragma once
#include "header.h"

class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript() {}

	void SetObject(const shared_ptr<NXObject>& pObject);

	virtual void Update() = 0;

protected:
	shared_ptr<NXObject> m_pObject;
};
