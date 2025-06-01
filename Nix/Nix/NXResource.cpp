#include "NXResource.h"

NXResource::NXResource(NXResourceType type, const std::string& name) :
	NXObject(name),
	m_type(type),
	m_resourceState(D3D12_RESOURCE_STATE_COPY_DEST)
{
}
