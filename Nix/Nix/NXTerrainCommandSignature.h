#pragma once
#include "NXInstance.h"
#include "BaseDefs/DX12.h"

// GPU Terrain专用的CommandSignature管理器
// 负责管理GPU Terrain的DrawIndexedInstanced indirect command signature
class NXTerrainCommandSignature : public NXInstance<NXTerrainCommandSignature>
{
public:
	NXTerrainCommandSignature();
	~NXTerrainCommandSignature() = default;

	void Init();

	const D3D12_COMMAND_SIGNATURE_DESC& GetDrawIndexArgDesc() const { return m_cmdSigDesc; }
	const D3D12_INDIRECT_ARGUMENT_DESC* GetIndirectArgDesc() const { return m_drawIndexArgDesc; }

private:
	D3D12_INDIRECT_ARGUMENT_DESC m_drawIndexArgDesc[1];
	D3D12_COMMAND_SIGNATURE_DESC m_cmdSigDesc;
};
