#include "NXEvent.h"

void NXEvent::AddListener(const shared_ptr<NXListener>& pListener)
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
