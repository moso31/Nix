#pragma once
#include "NXGraphicPass.h"
#include "NXComputePass.h"
#include "NXRGResource.h"

struct NXRGResourceSlot
{
	NXRGResource* resource;
	uint32_t slot;
};

class NXRenderGraph;
class NXConstantBufferImpl;
class NXRGPassNodeBase
{
public:
	NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRenderPass* pPass) :
		m_pRenderGraph(pRenderGraph), m_passName(passName), m_pPass(pPass) {}

	const std::string& GetName() { return m_passName; }

	// 设置Pass的根参数布局
	void SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount);

	// 设置Pass输入资源。
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// 设置Pass输入的CB
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer);

	// 设置pass输出RT。
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion);
	NXRGResource* WriteDS(NXRGResource* pResource, bool useOldVersion);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t outUAVIndex, bool useOldVersion);

	NXRenderPass* GetRenderPass() { return m_pPass; }

	void Compile(bool isResize);
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

	// 获取pass关联资源
	const std::vector<NXRGResourceSlot>& GetInputs() { return m_inputs; }
	const std::vector<NXRGResourceSlot>& GetOutputs() { return m_outputs; }

private:
	void Compile_GraphicsPass(bool isResize);
	void Compile_ComputePass(bool isResize);

protected:
	std::string m_passName;
	NXRenderGraph* m_pRenderGraph;
	NXRenderPass* m_pPass;

	// Pass记录自己依赖的资源，但生命周期由NXRenderGraph*管理；
	std::vector<NXRGResourceSlot> m_inputs; 
	std::vector<NXRGResourceSlot> m_outputs;

	// 记录当前pass的根参数布局
	NXRGRootParamLayout m_rootParamLayout; 
};

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName, NXRenderPass* pPass) : NXRGPassNodeBase(pRenderGraph, passName, pPass), m_passData(NXRGPassData()) {}

	NXRGPassData& GetData() { return m_passData; }

	void Execute(ID3D12GraphicsCommandList* pCmdList) override
	{
		m_executeFunc(pCmdList, m_passData);
		m_pPass->Render(pCmdList);
	}

	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> func) { m_executeFunc = func; }

private:
	NXRGPassData m_passData;

	// 如果需要在执行前进行一些pass操作（比如clearRT），可使用此方法的lambda。
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
