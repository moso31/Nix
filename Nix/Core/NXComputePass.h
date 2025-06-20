#pragma once
#include "NXRenderPass.h"
#include "NXBuffer.h"

struct NXRGResourceUAV
{
	NXRGResource* pRes;
	bool isUAVCounter;
};

class NXComputePass : public NXRenderPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	void InitCSO();
	void InitCommandSignature();

	void SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetInput(NXRGResource* pRes, uint32_t slotIndex);
	void SetOutput(NXRGResource* pRes, uint32_t slotIndex, bool IsUAVCounter = false);
	void SetIndirectArguments(NXRGResource* pRes);

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

	void CopyUAVCounterTo(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pUAVCounterRes);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC		m_csoDesc;
	ComPtr<ID3D12PipelineState>				m_pCSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;
	ComPtr<ID3D12CommandSignature>			m_pCommandSig;

	uint32_t 								m_threadGroupX;
	uint32_t 								m_threadGroupY;
	uint32_t 								m_threadGroupZ;

	std::vector<NXRGResource*>				m_pInRes;
	std::vector<NXRGResourceUAV>			m_pOutRes;
	NXRGResource*							m_pIndirectArgs;
};