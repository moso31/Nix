#pragma once
#include "header.h"

class NXScript
{
public:
	NXScript(const shared_ptr<NXObject>& pObject);
	virtual ~NXScript() {}

	void SetObject(const shared_ptr<NXObject>& pObject) { m_pObject = pObject; }

	virtual void Update() {};

protected:
	shared_ptr<NXObject> m_pObject;
};