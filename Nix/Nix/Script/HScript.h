#pragma once
#include "header.h"

enum HSCRIPTTYPE
{
	HSCRIPT_NONE,
	HSCRIPT_TEST,
	HSCRIPT_FIRST_PERSONAL_CAMERA
};

class NXScript
{
public:
	NXScript(const shared_ptr<NXObject>& pObject);
	virtual ~NXScript() {}

	void SetObject(shared_ptr<NXObject>& pObject) { m_pObject = pObject; }

	virtual void Update() {};

protected:
	shared_ptr<NXObject> m_pObject;
};