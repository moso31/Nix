#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"
#include "Ntr.h"

class NXScene;
class NXTexture2D;
class NXRenderTarget;
class NXColorMappingRenderer;
class NXEditorObjectRenderer
{
public:
	NXEditorObjectRenderer(NXScene* pScene);
	~NXEditorObjectRenderer();

	void Init();
	void OnResize(const Vector2& rtSize);
	void Render();

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

private:
	bool m_bEnable;
	NXScene* m_pScene;

	Ntr<NXTexture2D>					m_pTexPassOut;

	MultiFrame<CommittedResourceData<ConstantBufferVector4>>	m_cbParams;

	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;
};
