#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"
#include "Ntr.h"

class NXScene;
class NXRenderTarget;
class NXTexture2D;
class NXSubSurfaceRenderer
{
public:
	NXSubSurfaceRenderer(NXScene* pScene);
	~NXSubSurfaceRenderer();

	void Init();
	void Render();

	void Release();

private:
	void RenderSSSSS();

private:
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator;

	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	Ntr<NXTexture2D> m_pTexPassIn[6];
	Ntr<NXTexture2D> m_pTexPassOut;
	Ntr<NXTexture2D> m_pTexDepth;

	NXScene* m_pScene;
};
