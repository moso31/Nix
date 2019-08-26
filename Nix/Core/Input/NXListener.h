#pragma once
#include "header.h"

class NXListener
{
public:
	NXListener(const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc);
	virtual ~NXListener() {}

	void SetFunc(const function<void(NXEventArg)>& pFunc);
	function<void(NXEventArg)> GetFunc() const;

private:
	shared_ptr<NXObject> m_pObject;
	function<void(NXEventArg)> m_pFunc;
};