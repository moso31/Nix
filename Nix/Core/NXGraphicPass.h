#pragma once
#include "NXRenderPass.h"

class NXPassTexture
{
public:
	NXPassTexture(Ntr<NXTexture> pTexture, uint32_t passSlotIndex = -1) :
		pTexture(pTexture), slotIndex(passSlotIndex) {}

	Ntr<NXTexture> pTexture;
	uint32_t slotIndex = -1;
};

class NXGraphicPass : public NXRenderPass
{
public:
	NXGraphicPass();
	virtual ~NXGraphicPass() {}

	virtual void SetupInternal() = 0;

	void SetInputTex(NXRGResource* pTex, uint32_t slotIndex, uint32_t spaceIndex = 0);
	void SetOutputRT(NXRGResource* pTex, uint32_t rtIndex);
	void SetOutputDS(NXRGResource* pTex);

	NXRGResource* GetInputTex(uint32_t slotIndex, uint32_t spaceIndex = 0) { return m_pInTexs[spaceIndex][slotIndex]; }
	NXRGResource* GetOutputRT(uint32_t index) { return m_pOutRTs[index]; }
	NXRGResource* GetOutputDS() { return m_pOutDS; }

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetRenderTargetMesh(const std::string& rtSubMeshName);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

	void SetStencilRef(const UINT stencilRef) { m_stencilRef = stencilRef; }

	// 设置PSO
	// 在所有内容设置完毕后，再调用这个函数，创建PSO
	void InitPSO();

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render();

	void Release() {}

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	UINT m_stencilRef;
	Vector2 m_viewPortSize;

	// [space][slot]
	std::vector<std::vector<NXRGResource*>>	m_pInTexs;
	std::vector<NXRGResource*>				m_pOutRTs;
	NXRGResource*							m_pOutDS;

	// rt 使用的 subMesh 的名称。
	// 实际渲染时根据这个名字确定使用 NXSubMeshGeometryEditor 的哪个 subMesh 作为 RT.
	// 一般不需要设置，就用默认的 "_RenderTarget" 就行，但也有特殊情况，
	// 比如渲染 CubeMap 的 Pass，需要设置为 "_CubeMapSphere"，使用一张圆形的 RT。
	std::string								m_rtSubMeshName;
};
