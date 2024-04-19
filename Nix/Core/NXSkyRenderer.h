#pragma once
#include "BaseDefs/DX12.h"
#include "Ntr.h"
#include "ShaderStructures.h"

class NXScene;
class NXTexture2D;
class NXSkyRenderer
{
public:
	NXSkyRenderer(NXScene* pScene);
	~NXSkyRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release() {}

private:
	void InitSignature();
	void InitPSO();

private:
	ComPtr<ID3D12RootSignature>			m_pRootSig;
	ComPtr<ID3D12PipelineState>			m_pPSO;

	Ntr<NXTexture2D>	m_pTexPassOut;
	Ntr<NXTexture2D>	m_pTexPassOutDepth;

	NXScene* m_pScene;
};
