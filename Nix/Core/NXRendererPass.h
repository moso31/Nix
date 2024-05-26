#pragma once
#include "BaseDefs/DX12.h"

class NXRendererPass
{
public:
	NXRendererPass() {}
	~NXRendererPass() {}

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

protected:
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;
};
