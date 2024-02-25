#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"

class NXScene;
class NXGBufferRenderer
{
private:
	explicit NXGBufferRenderer() = default;
public:
	NXGBufferRenderer(NXScene* pScene);
	~NXGBufferRenderer();

	void Init();
	void Render();

	void Release();

private:
	Ntr<NXTexture2D> m_pDepthZ;
	Ntr<NXTexture2D> m_pGBufferRT[4];

	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	NXScene* m_pScene;
};
