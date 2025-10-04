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

	void SetInputTex(NXRGResource* pTex, uint32_t slotIndex);
	void SetOutputRT(NXRGResource* pTex, uint32_t rtIndex);
	void SetOutputDS(NXRGResource* pTex);

	NXRGResource* GetInputTex(uint32_t slotIndex) { return m_pInTexs[slotIndex]; }
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

	// ����PSO
	// ����������������Ϻ��ٵ����������������PSO
	void InitPSO();

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render();

	void Release() {}

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;
	UINT m_stencilRef;
	Vector2 m_viewPortSize;

	std::vector<NXRGResource*>				m_pInTexs;
	std::vector<NXRGResource*>				m_pOutRTs;
	NXRGResource*							m_pOutDS;

	// rt ʹ�õ� subMesh �����ơ�
	// ʵ����Ⱦʱ�����������ȷ��ʹ�� NXSubMeshGeometryEditor ���ĸ� subMesh ��Ϊ RT.
	// һ�㲻��Ҫ���ã�����Ĭ�ϵ� "_RenderTarget" ���У���Ҳ�����������
	// ������Ⱦ CubeMap �� Pass����Ҫ����Ϊ "_CubeMapSphere"��ʹ��һ��Բ�ε� RT��
	std::string								m_rtSubMeshName;
};
