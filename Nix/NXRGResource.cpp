#include "NXRGResource.h"

NXRGResource::NXRGResource(NXRGResource* pOldResource) :
	m_bHasWrited(false)
{
	m_handle = new NXRGHandle(pOldResource->GetHandle());
	m_description = pOldResource->GetDescription();
	m_name = pOldResource->GetName() + "_" + std::to_string(m_handle->GetIndex()) + "_" + std::to_string(m_handle->GetVersion());
}

NXRGResource::NXRGResource(const std::string& name, const NXRGDescription& description) :
	m_description(description),
	m_bHasWrited(false),
	m_name(name)
{
	m_handle = new NXRGHandle();
}
