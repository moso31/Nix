#include "NXResource.h"

NXResource::NXResource(NXResourceType type, const std::string& name) :
	NXObject(name),
	m_type(type),
	m_resourceState(D3D12_RESOURCE_STATE_COMMON)
{
}

void NXResource::SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& srcState, const D3D12_RESOURCE_STATES& dstState)
{
	m_resourceState = srcState;
	SetResourceState(pCommandList, dstState);
}

bool NXResource::IsTexture() const
{
	return m_type == NXResourceType::Tex1D ||
		m_type == NXResourceType::Tex2D ||
		m_type == NXResourceType::TexCube ||
		m_type == NXResourceType::Tex2DArray ||
		m_type == NXResourceType::Tex3D;
}
