#include "NXRGResource.h"

NXRGResource::NXRGResource(NXRGHandle* handle)
{
	m_handle = new NXRGHandle(handle);
}

NXRGResource::NXRGResource(const NXRGDescription& description) :
	m_description(description)
{
	m_handle = new NXRGHandle();
}
