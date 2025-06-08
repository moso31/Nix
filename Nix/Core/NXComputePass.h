#pragma once
#include "NXRenderPass.h"
#include "NXBuffer.h"

class NXComputePass : public NXRenderPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	void InitCSO();

	void SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetInput(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInput(const Ntr<NXResource>& pTex, uint32_t slotIndex);
	void SetOutput(const Ntr<NXResource>& pTex, uint32_t slotIndex);

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC		m_csoDesc;
	ComPtr<ID3D12PipelineState>				m_pCSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;

	uint32_t 								m_threadGroupX;
	uint32_t 								m_threadGroupY;
	uint32_t 								m_threadGroupZ;

	std::vector<Ntr<NXResource>>			m_pInRes;
	std::vector<Ntr<NXResource>>			m_pOutRes;
};