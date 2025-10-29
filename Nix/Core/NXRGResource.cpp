#include "NXRGResource.h"

NXRGResource::NXRGResource(NXRGResource* pOldResource) :
	m_bHasWrited(false)
{
	m_version = new NXRGResourceVersion(pOldResource->GetVersion());
	m_description = pOldResource->GetDescription();
	m_name = pOldResource->GetName() + "_" + std::to_string(m_version->GetIndex()) + "_" + std::to_string(m_version->GetVersion());
}

NXRGResource::NXRGResource(const std::string& name, const NXRGDescription& description) :
	m_description(description),
	m_bHasWrited(false),
	m_name(name)
{
	m_version = new NXRGResourceVersion();
}
