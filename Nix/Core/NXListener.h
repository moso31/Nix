#pragma once
#include "header.h"

class NXListener
{
public:
	NXListener(NXObject* pObject, const std::function<void(NXEventArg)>& pFunc);
	virtual ~NXListener() {}

	void SetFunc(const std::function<void(NXEventArg)>& pFunc);
	std::function<void(NXEventArg)> GetFunc() const;

	void Release();

private:
	NXObject* m_pObject;
	std::function<void(NXEventArg)> m_pFunc;
};