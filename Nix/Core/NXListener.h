#pragma once
#include "header.h"

// 事件监听器。
class NXListener
{
public:
	NXListener(NXObject* pObject, const std::function<void(NXEventArgs)>& pFunc);
	virtual ~NXListener() {}

	void SetFunc(const std::function<void(NXEventArgs)>& pFunc);
	std::function<void(NXEventArgs)> GetFunc() const;

	// event notify到当前listener时，将执行此方法。
	// 2021.2.22 当前实际仅执行m_pFunc函数。
	void Update(const NXEventArgs& args);
	void Release();

private:
	NXObject* m_pObject;

	std::function<void(NXEventArgs)> m_pFunc;
};
