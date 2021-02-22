#include "NXListener.h"
#include "NXEventArgs.h"

NXListener::NXListener(NXObject* pObject, const std::function<void(NXEventArgs)>& pFunc) :
	m_pObject(pObject),
	m_pFunc(pFunc)
{
}

void NXListener::SetFunc(const std::function<void(NXEventArgs)>& pFunc)
{
	m_pFunc = pFunc;
}

std::function<void(NXEventArgs)> NXListener::GetFunc() const
{
	return m_pFunc;
}

void NXListener::Update(const NXEventArgs& args)
{
	m_pFunc(args);
}

void NXListener::Release()
{
}
