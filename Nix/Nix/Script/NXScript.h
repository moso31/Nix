#pragma once
#include "header.h"

class NXScript
{
public:
	NXScript() {}
	virtual ~NXScript();

	void SetObject(const std::shared_ptr<NXObject>& pObject);

	virtual void Update() = 0;

protected:
	std::shared_ptr<NXObject> m_pObject;
};
