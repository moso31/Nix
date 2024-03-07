#pragma once
#include "BaseDefs/DX12.h"
#include "Ntr.h"
#include "ShaderStructures.h"

class NXScene;
class NXBRDFLut;
class NXTexture2D;
class NXForwardRenderer
{
public:
	NXForwardRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXForwardRenderer();

	void Init();
	void Render();

	void Release() {};

private:
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> 					m_pTexPassIn[9];
	Ntr<NXTexture2D> 					m_pTexPassOut;
	Ntr<NXTexture2D> 					m_pTexDepth;

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
