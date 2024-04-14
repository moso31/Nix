#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"
#include "Ntr.h"

class NXScene;
class NXBRDFLut;
class NXTexture2D;
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
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator;

	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> 					m_pTexPassIn[9];
	Ntr<NXTexture2D> 					m_pTexPassOut[4];

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
