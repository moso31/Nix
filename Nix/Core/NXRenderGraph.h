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

	// 当前资源在pass作为输入
	// handle不变，仍返回resID。
	NXRGHandle Read(NXRGHandle resID, NXRGPassNodeBase* passNode);

	// 当前资源在pass作为输出。
	// 如果是Import资源，则返回同一资源的handle；
	// 如果是Create资源，则创建新版本资源并返回新handle
	NXRGHandle Write(NXRGPassNodeBase* passNode,NXRGHandle resID);

	// 当前资源在pass作为叠加读写（之前的pass已经有结果，这里只是叠加新的内容）。
	// handle不变，仍返回resID
	NXRGHandle ReadWrite(NXRGPassNodeBase* passNode, NXRGHandle resID);

	// 定义一个创建类资源
	NXRGHandle Create(const std::string& name, const NXRGDescription& desc);
	// 导入一个外部资源
	NXRGHandle Import(const Ntr<NXResource>& importResource);

	void Compile();
	void Execute();

	void Clear();

	// 获取资源和pass的接口
	Ntr<NXResource> GetResource(NXRGHandle handle);
	Ntr<NXResource> GetUsingResourceByName(const std::string& name); // 通过资源名称查找资源；仅调试使用

	const std::vector<NXRGPassNodeBase*>& GetPassNodes() { return m_passNodes; }
	const std::unordered_map<NXRGHandle, NXRGResource*>& GetResourceMap() { return m_resourceMap; }

	// GUI数据接口
	const std::vector<NXRGGUIResource>& GetGUIVirtualResources() const { return m_guiVirtualResources; }
	const std::vector<NXRGGUIResource>& GetGUIPhysicalResources() const { return m_guiPhysicalResources; }
	const std::vector<NXRGGUIResource>& GetGUIImportedResources() const { return m_guiImportedResources; }
	int GetMaxTimeLayer() const { return m_maxTimeLayer; }

private:
	void GenerateGUIData();

	Ntr<NXResource> CreateResourceByDescription(const NXRGDescription& desc, NXRGHandle handle);

private:
	// 静态变量记录纹理编号
	static uint32_t s_resourceId;

	// 图依赖的所有pass
	std::vector<NXRGPassNodeBase*> m_passNodes;

	// 记录RGHandle和RGResource的映射关系
	std::unordered_map<NXRGHandle, NXRGResource*> m_resourceMap;

	// NXRGHandle-导入资源 映射
	std::unordered_map<NXRGHandle, Ntr<NXResource>> m_importedResourceMap; 

	// Compile相关：
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

	// 记录实际资源到RGHandle的反映射
	std::unordered_map<Ntr<NXResource>, std::vector<NXRGHandle>> m_allocatedHandlesMap; 
	std::unordered_map<Ntr<NXResource>, NXRGHandle> m_importedHandlesMap;

	// 记录每种desc的资源在 上帧+本帧 的分配情况，以确认资源复用
	std::unordered_map<NXRGDescription, std::vector<Ntr<NXResource>>> m_lastResourceUsingMap;
	std::unordered_map<NXRGDescription, std::vector<Ntr<NXResource>>> m_resourceUsingMap;

	// GUI数据
	std::vector<NXRGGUIResource> m_guiVirtualResources;
	std::vector<NXRGGUIResource> m_guiPhysicalResources; // Create资源
	std::vector<NXRGGUIResource> m_guiImportedResources; // Import资源
	int m_maxTimeLayer = 0;
};
