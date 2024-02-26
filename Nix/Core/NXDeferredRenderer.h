#pragma once
#include "BaseDefs/DX11.h"
#include "ShaderStructures.h"

class NXScene;
class NXBRDFLut;
class NXRenderTarget;
class NXDeferredRenderer
{
public:
	NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXDeferredRenderer();

	void Init();
	void Render();

	void Release();

private:
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> 					m_pTexPassIn[9];
	Ntr<NXTexture2D> 					m_pTexPassOut[4];

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
