#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "SimpleMath.h"
#include "Ntr.h"
#include "NXObject.h"
#include "NXSerializable.h"
#include "NXTextureDefinitions.h"

class NXResource : public NXObject, public NXSerializable
{
public:
	NXResource(NXResourceType type, const std::string& name = "");
	virtual ~NXResource() = default;

	virtual const D3D12_RESOURCE_STATES& GetResourceState() { return m_resourceState; }
	virtual void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state) = 0;

protected:
	D3D12_RESOURCE_STATES m_resourceState;
	NXResourceType m_type;
};
