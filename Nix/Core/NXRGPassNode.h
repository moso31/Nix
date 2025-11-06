#pragma once
#include "NXRGUtil.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"

class NXRenderGraph;
class NXRGPassNodeBase
{
public:
	NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName) :
		m_pRenderGraph(pRenderGraph),
		m_passName(passName)
	{
	}

	const std::string& GetName() { return m_passName; }

	virtual void Setup(NXRGBuilder& builder) = 0;
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

	void AddInput(NXRGHandle resHandle) { m_inputs.push_back(resHandle); }
	void AddOutput(NXRGHandle resHandle) { m_outputs.push_back(resHandle); }

	// 获取pass关联资源
	const std::vector<NXRGHandle>& GetInputs() { return m_inputs; }
	const std::vector<NXRGHandle>& GetOutputs() { return m_outputs; }

protected:
	std::string m_passName;
	NXRenderGraph* m_pRenderGraph;

	// Pass记录自己依赖的资源指针（但不负责其生命周期）
	std::vector<NXRGHandle> m_inputs; 
	std::vector<NXRGHandle> m_outputs;
};

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName) : NXRGPassNodeBase(pRenderGraph, passName), m_passData(NXRGPassData()) {}
	NXRGPassData& GetData() { return m_passData; }

	void Setup(NXRGBuilder& builder) override
	{
		m_setupFunc(builder, m_passData);
	}

	void Execute(ID3D12GraphicsCommandList* pCmdList) override
	{
		NX12Util::BeginEvent(pCmdList, m_passName.c_str());

		m_executeFunc(pCmdList, m_passData);

		NX12Util::EndEvent(pCmdList);
	}

	void RegisterSetupFunc(std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> func) { m_setupFunc = std::move(func); }
	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> func) { m_executeFunc = std::move(func); }

private:
	NXRGPassData m_passData;

	std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> m_setupFunc;
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
