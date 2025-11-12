#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"

#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

class NXResource;
class NXRGResource;

struct NXRGAllocedResourceLifeTimes
{
	Ntr<NXResource> pResource; // 分配的资源指针
	std::vector<NXRGLifeTime> descLifeTimes; // 此资源占用的时间段列表
};

class NXRenderGraph
{
public:
	NXRenderGraph() {}
	virtual ~NXRenderGraph() {}

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddPass(const std::string& name,
		std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup,
		std::function<void(ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& frameResources, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name);

		// setup
		NXRGBuilder builder(this, pPassNode);
		setup(builder, pPassNode->GetData());

		// regist execute
		pPassNode->RegisterExecuteFunc(std::move(execute));

		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	NXRGHandle Read(NXRGHandle resID, NXRGPassNodeBase* passNode);
	NXRGHandle Write(NXRGPassNodeBase* passNode,NXRGHandle resID);

	NXRGHandle Create(const std::string& name, const NXRGDescription& desc);
	NXRGHandle Import(const Ntr<NXResource>& importResource);

	void Compile();
	void Execute();

	void Clear();

	// 获取资源和pass的接口
	Ntr<NXResource> GetResource(NXRGHandle handle);
	Ntr<NXResource> GetUsingResourceByName(const std::string& name); // 通过资源名称查找资源；仅调试使用

	const std::vector<NXRGPassNodeBase*>& GetPassNodes() { return m_passNodes; }
	const std::unordered_map<NXRGHandle, NXRGResource*>& GetResourceMap() { return m_resourceMap; }

private:
	Ntr<NXResource> CreateResourceByDescription(const NXRGDescription& desc, NXRGHandle handle);

private:
	// 图依赖的所有pass
	std::vector<NXRGPassNodeBase*> m_passNodes;

	// 记录RGHandle和RGResource的映射关系
	std::unordered_map<NXRGHandle, NXRGResource*> m_resourceMap;

	// NXRGHandle-导入资源 映射
	std::unordered_map<NXRGHandle, Ntr<NXResource>> m_importedResourceMap; 

	// Compile相关：
	std::unordered_map<NXRGHandle, NXRGPassNodeBase*> m_resourceProducerPassMap; // 每个RGHandle的生产者Pass
	// Compile-拓扑排序相关：
	std::unordered_map<NXRGPassNodeBase*, std::vector<NXRGPassNodeBase*>> m_adjTablePassMap; // 每个Pass的邻接表
	std::unordered_map<NXRGPassNodeBase*, int> m_indegreePassMap; // 每个Pass的入度
	std::unordered_map<NXRGPassNodeBase*, int> m_timeLayerPassMap; // 每个Pass的优先顺序，越小越先执行
	// Compile-资源生命周期管理相关：
	std::unordered_map<NXRGHandle, NXRGLifeTime> m_resourceLifeTimeMap; // 每个资源的起止时间
	// Compile-贪心确认资源分配方案
	// 如果desc一样，并且lifetime不重合，那么可以复用
	std::unordered_map<NXRGDescription, std::vector<NXRGAllocedResourceLifeTimes>> m_descLifeTimesMap; // 每种desc实际覆盖的生命周期，二维vector=[资源实例][覆盖生命周期段]
	// NXRGHandle-实际分配的资源 映射
	std::unordered_map<NXRGHandle, Ntr<NXResource>> m_allocatedResourceMap; 

	// 记录每种desc的资源在 上帧+本帧 的分配情况，以确认资源复用
	std::unordered_map<NXRGDescription, std::vector<Ntr<NXResource>>> m_lastResourceUsingMap;
	std::unordered_map<NXRGDescription, std::vector<Ntr<NXResource>>> m_resourceUsingMap;

	// 静态变量记录纹理编号
	static uint32_t s_resourceId;
};
