#include "NXEvent.h"

NXEvent::~NXEvent()
{
}

void NXEvent::AddListener(NXListener* pListener)
{
	m_listeners.push_back(pListener);
}

void NXEvent::OnNotify(NXEventArg eArg)
{
	for (auto listener : m_listeners)
	{
		listener->GetFunc()(eArg);
	}
}

void NXEvent::Release()
{
	for (auto listener : m_listeners) 
		SafeRelease(listener);
}
