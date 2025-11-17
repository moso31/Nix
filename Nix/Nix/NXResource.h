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

	virtual void Serialize() override {};
	virtual void Deserialize() override {};

	virtual uint32_t GetWidth()		const = 0;
	virtual uint32_t GetHeight()	const = 0;
	virtual uint32_t GetArraySize()	const = 0;
	virtual uint32_t GetMipLevels()	const = 0;

	NXResourceType GetResourceType() const { return m_type; }
	bool IsBuffer() const { return m_type == NXResourceType::Buffer; }
	bool IsTexture() const;

	virtual ID3D12Resource* GetD3DResource() const = 0;
	virtual ID3D12Resource* GetD3DResourceUAVCounter() const { return nullptr; }

	virtual const D3D12_RESOURCE_STATES& GetResourceState() { return m_resourceState; }
	virtual void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state) = 0;
	virtual void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& srcState, const D3D12_RESOURCE_STATES& dstState);

protected:
	D3D12_RESOURCE_STATES m_resourceState;
	NXResourceType m_type;
};
