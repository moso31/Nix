#include "NXEvent.h"

NXEvent::~NXEvent()
{
	for (auto it = m_listeners.begin(); it != m_listeners.end(); it++)
	{
		(*it).reset();
	}
}

void NXEvent::AddListener(const std::shared_ptr<NXListener>& pListener)
{
	m_listeners.push_back(pListener);
}

void NXEvent::OnNotify(NXEventArg eArg)
{
	for (auto it = m_listeners.begin(); it != m_listeners.end(); it++)
	{
		(*it)->GetFunc()(eArg);
	}
}

void NXEvent::Release()
{
	for (auto it = m_listeners.begin(); it != m_listeners.end(); it++)
	{
		(*it)->Release();
	}
}
