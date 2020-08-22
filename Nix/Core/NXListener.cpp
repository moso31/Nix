#include "NXListener.h"

NXListener::NXListener(const std::shared_ptr<NXObject>& pObject, const std::function<void(NXEventArg)>& pFunc) :
	m_pObject(pObject),
	m_pFunc(pFunc)
{
}

void NXListener::SetFunc(const std::function<void(NXEventArg)>& pFunc)
{
	m_pFunc = pFunc;
}

std::function<void(NXEventArg)> NXListener::GetFunc() const
{
	return m_pFunc;
}

void NXListener::Release()
{
}
