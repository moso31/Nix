#pragma once
#include "BaseDefs/DX12.h"
#include "Ntr.h"
#include "ShaderStructures.h"

class NXDepthRenderer
{
public:
	NXDepthRenderer() {}
	~NXDepthRenderer() {}

	void Init();
	void Render();

	void Release();

private:
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> 					m_pTexPassIn;
	Ntr<NXTexture2D> 					m_pTexPassOut;
};
