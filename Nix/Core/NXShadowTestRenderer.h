#pragma once
#include "BaseDefs/DX12.h"
#include "Ntr.h"
#include "ShaderStructures.h"

class NXTexture2DArray;
class NXShadowTestRenderer
{
public:
	NXShadowTestRenderer() = default;
	~NXShadowTestRenderer();

	void Init();
	void Render();

	void SetShadowMapDepth(const Ntr<NXTexture2D>& pShadowMapDepth) { m_pTexPassIn1 = pShadowMapDepth; }

	void Release();

private:
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> m_pTexPassIn0; // sceneDepth
	Ntr<NXTexture2D> m_pTexPassIn1; // shadowMapDepth
	Ntr<NXTexture2D> m_pTexPassOut;
};
