#include "NXRGResource.h"

NXRGResource::NXRGResource(NXRGResource* pOldResource) :
	m_bHasWrited(false)
{
	m_handle = new NXRGHandle(pOldResource->GetHandle());
	m_description = pOldResource->GetDescription();
}

NXRGResource::NXRGResource(const NXRGDescription& description) :
	m_description(description),
	m_bHasWrited(false)
{
	m_handle = new NXRGHandle();
}
