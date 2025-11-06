#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"

#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

class NXResource;
class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddPass(const std::string& name, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pRendererPass);
		pPassNode->RegisterSetupFunc(std::move(setup));
		pPassNode->RegisterExecuteFunc(std::move(execute));
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddComputePass(const std::string& name, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pComputePass);
		pPassNode->RegisterSetupFunc(std::move(setup));
		pPassNode->RegisterExecuteFunc(std::move(execute));
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddReadbackBufferPass(const std::string& name, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pReadbackPass);
		pPassNode->RegisterSetupFunc(std::move(setup));
		pPassNode->RegisterExecuteFunc(std::move(execute));
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	NXRGHandle Read(NXRGHandle resID, NXRGPassNodeBase* passNode);
	NXRGHandle Write(NXRGPassNodeBase* passNode,NXRGHandle resID);

	NXRGHandle Create(const std::string& name, const NXRGDescription& desc);
	NXRGHandle Import(const Ntr<NXResource>& importResource);

	void Compile();
	void Execute() {}

	// 获取资源和pass的接口
	const std::vector<NXRGResource*>& GetResources() { return m_resourceNodes; }
	const std::vector<NXRGPassNodeBase*>& GetPassNodes() { return m_passNodes; }

	void Destroy();

private:
	// 图依赖的所有pass
	std::vector<NXRGPassNodeBase*> m_passNodes;

	// NXRG自己创建的资源（如RT）
	std::vector<NXRGResource*> m_resourceNodes;

	// 记录RGHandle和实际资源的映射关系
	std::map<NXRGHandle, NXRGResource*> m_resourceMap;

	// 外部导入的资源
	std::vector<NXRGResource*> m_externalNodes;
};
