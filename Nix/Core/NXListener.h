#pragma once
#include "header.h"

class NXListener
{
public:
	NXListener(const std::shared_ptr<NXObject>& pObject, const std::function<void(NXEventArg)>& pFunc);
	virtual ~NXListener() {}

	void SetFunc(const std::function<void(NXEventArg)>& pFunc);
	std::function<void(NXEventArg)> GetFunc() const;

	void Release();

private:
	std::shared_ptr<NXObject> m_pObject;
	std::function<void(NXEventArg)> m_pFunc;
};