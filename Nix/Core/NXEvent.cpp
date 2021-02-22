#include "NXEvent.h"

NXEvent::~NXEvent()
{
}

void NXEvent::AddListener(NXListener* pListener)
{
	m_listeners.push_back(pListener);
}

void NXEvent::Notify(const NXEventArgs& eArgs)
{
	for (auto listener : m_listeners)
	{
		listener->Update(eArgs);
	}
}

void NXEvent::Release()
{
	for (auto listener : m_listeners) 
		SafeRelease(listener);
}
