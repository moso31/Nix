#include "NXRefCountable.h"

void NXRefCountable::IncRef()
{
	++m_refCount;
}

void NXRefCountable::DecRef()
{
	if (--m_refCount == 0)
	{
		DEBUG_ACTION(if (!m_refCountDebug.empty()) printf_s("%s removing.\n", m_refCountDebug.c_str()));
		delete this;
	}
}
