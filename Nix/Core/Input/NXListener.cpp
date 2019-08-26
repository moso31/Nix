#include "NXListener.h"

NXListener::NXListener(const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc) :
	m_pObject(pObject),
	m_pFunc(pFunc)
{
}

void NXListener::SetFunc(const function<void(NXEventArg)>& pFunc)
{
	m_pFunc = pFunc;
}

function<void(NXEventArg)> NXListener::GetFunc() const
{
	return m_pFunc;
}
