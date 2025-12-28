#include "NXRenderGraph.h"
#include "NXTexture.h"
#include "NXBuffer.h"
#include "NXRGResource.h"
#include "NXGlobalDefinitions.h"
#include "NXAllocatorManager.h"
#include <limits>
#include <algorithm>

uint32_t NXRenderGraph::s_resourceId = 0;

NXRGHandle NXRenderGraph::Read(NXRGHandle resID, NXRGPassNodeBase* passNode)
{
	passNode->AddInput(resID);
	m_handlePassNodes[resID].insert(passNode);
	return resID;
}

NXRGHandle NXRenderGraph::Write(NXRGPassNodeBase* passNode, NXRGHandle resID)
{
	if (m_resourceMap[resID]->IsImported())
	{
		// 导入资源不创建新版本，直接复用
		passNode->AddOutput(resID); 
		m_handlePassNodes[resID].insert(passNode);
		return resID;
	}

	NXRGResource* pResource = new NXRGResource(m_resourceMap[resID]);
	NXRGHandle handle = pResource->GetHandle();

	passNode->AddOutput(handle);
	m_resourceMap[handle] = pResource;
	m_handlePassNodes[handle].insert(passNode);

	return handle;
}

NXRGHandle NXRenderGraph::ReadWrite(NXRGPassNodeBase* passNode, NXRGHandle resID)
{
	passNode->AddInput(resID);
	passNode->AddOutput(resID);
	m_handlePassNodes[resID].insert(passNode);
	return resID;
}

NXRGHandle NXRenderGraph::Create(const std::string& name, const NXRGDescription& desc)
{
	NXRGResource* pResource = new NXRGResource(name, desc, false);
	NXRGHandle handle = pResource->GetHandle();

	m_resourceMap[handle] = pResource;
	return handle;
}

NXRGHandle NXRenderGraph::Import(const Ntr<NXResource>& importResource)
{
	NXRGDescription desc;
	NXRGResource* pResource = new NXRGResource(importResource->GetName(), desc, true);
	NXRGHandle handle = pResource->GetHandle();

	m_resourceMap[handle] = pResource;
	m_importedResourceMap[handle] = importResource;
	m_importedHandlesMap[importResource] = handle;
	return handle;
}

void NXRenderGraph::Compile()
{
	s_resourceId = 0;

	for (auto& pass : m_passNodes)
	{
		m_indegreePassMap[pass] = 0; // 初始化入度表=0
		m_timeLayerPassMap[pass] = 0; // 初始化时间层级表=0
		pass->ClearBeforeCompile();
	}

	std::unordered_map<NXRGHandle, NXRGPassNodeBase*> lastWriteMap; // 构建邻接表用，记录每个RGHandle的最后写入Pass
	for (auto& pass : m_passNodes)
	{
		for (auto& inResID : pass->GetInputs())
		{
			auto it = lastWriteMap.find(inResID);
			if (it != lastWriteMap.end())
			{
				// 构建一条从最后写入该资源的Pass到当前Pass的边
				auto& lastWritePass = it->second;
				m_adjTablePassMap[lastWritePass].push_back(pass);
				m_indegreePassMap[pass]++; // 入度+1
			}
		}

		for (auto& outResID : pass->GetOutputs())
		{
			lastWriteMap[outResID] = pass;
		}
	}

	std::queue<NXRGPassNodeBase*> passQueue;
	for (auto& pass : m_passNodes)
	{
		if (m_indegreePassMap[pass] == 0)
		{
			passQueue.push(pass);
		}
	}

	// khan 拓扑排序
	while (!passQueue.empty())
	{
		auto* pass = passQueue.front();
		passQueue.pop();

		for (auto& neighborPass : m_adjTablePassMap[pass])
		{
			m_timeLayerPassMap[neighborPass] = std::max(m_timeLayerPassMap[neighborPass], m_timeLayerPassMap[pass] + 1);
			m_indegreePassMap[neighborPass]--;
			if (m_indegreePassMap[neighborPass] == 0)
			{
				passQueue.push(neighborPass);
			}
		}
	}

	// 确认每个resource的起止time
	for (auto& [resID, resNode] : m_resourceMap)
	{
		if (resNode->IsImported()) continue; // import资源不参与生命周期管理
		m_resourceLifeTimeMap[resID] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), nullptr, nullptr };
	}
	for (auto& pass : m_passNodes)
	{
		int passTimeLayer = m_timeLayerPassMap[pass]; // 当前pass的时间层级

		for (auto& resID : pass->GetInputs())
		{
			auto it = m_resourceLifeTimeMap.find(resID);
			if (it == m_resourceLifeTimeMap.end()) continue;

			if (it->second.start > passTimeLayer)
			{
				it->second.start = passTimeLayer;
				it->second.pStartPass = pass;
			}

			if (it->second.end < passTimeLayer)
			{
				it->second.end = passTimeLayer;
				it->second.pEndPass = pass;
			}
		}

		for (auto& resID : pass->GetOutputs())
		{
			auto it = m_resourceLifeTimeMap.find(resID);
			if (it == m_resourceLifeTimeMap.end()) continue;

			if (it->second.start > passTimeLayer)
			{
				it->second.start = passTimeLayer;
				it->second.pStartPass = pass;
			}

			if (it->second.end < passTimeLayer)
			{
				it->second.end = passTimeLayer;
				it->second.pEndPass = pass;
			}
		}
	}

	// 贪心，确认实际应该分配的资源
	for (auto& [resID, lifeTime] : m_resourceLifeTimeMap)
	{
		if (lifeTime.start == std::numeric_limits<int>::max() || lifeTime.end == std::numeric_limits<int>::min())
			continue; // 未被任何pass使用的资源不分配

		//if (m_resourceMap.find(resID) == m_resourceMap.end()) continue; // 不需要这句;resourceLifeTimeMap一定是resourceMap的子集
		auto& resourceDesc = m_resourceMap[resID]->GetDescription(); 

		bool bFoundReusable = false;
		if (m_descLifeTimesMap.find(resourceDesc) != m_descLifeTimesMap.end())
		{
			// 当前desc覆盖的生命周期
			for (auto& singleResourceLifeTime : m_descLifeTimesMap[resourceDesc]) 
			{
				// 检查desc的所有生命周期，看看和liftTime是否重合
				bool bCanReuse = true;
				for (auto& resourceLifeTime : singleResourceLifeTime.descLifeTimes)
				{
					// 和任意一个生命周期相交，就不能复用
					if (!(lifeTime.end < resourceLifeTime.start || lifeTime.start > resourceLifeTime.end))
					{
						bCanReuse = false;
						break;
					}
				}

				if (bCanReuse)
				{
					bFoundReusable = true;
					// 若可以复用，直接加入该实际资源实例的生命周期列表
					singleResourceLifeTime.descLifeTimes.push_back(lifeTime);
					m_allocatedResourceMap[resID] = singleResourceLifeTime.pResource; 
					m_allocatedHandlesMap[singleResourceLifeTime.pResource].push_back(resID);
					break;
				}
			}
		}

		if (!bFoundReusable) 
		{
			// 如果没有找到可复用的实例 需要新建一个
			NXRGAllocedResourceLifeTimes singleResourceLifeTime;
			singleResourceLifeTime.pResource = CreateResourceByDescription(resourceDesc, resID);
			singleResourceLifeTime.descLifeTimes.push_back(lifeTime);
			m_descLifeTimesMap[resourceDesc].push_back(singleResourceLifeTime);
			m_allocatedResourceMap[resID] = singleResourceLifeTime.pResource;
			m_allocatedHandlesMap[singleResourceLifeTime.pResource].push_back(resID);
		}
	}

	// 分发NXRGHandle到每个passNode.NXRGFrameResources
	for (auto& pass : m_passNodes)
	{
		for (auto& handle : pass->GetInputs())
		{
			auto& desc = m_resourceMap[handle]->GetDescription();
			if (m_resourceMap[handle]->IsImported())
			{
				pass->RegisterFrameResource(handle, m_importedResourceMap[handle]);
			}
			else
			{
				pass->RegisterFrameResource(handle, m_allocatedResourceMap[handle]);
			}
		}

		for (auto& handle : pass->GetOutputs())
		{
			auto& desc = m_resourceMap[handle]->GetDescription();
			if (m_resourceMap[handle]->IsImported())
			{
				pass->RegisterFrameResource(handle, m_importedResourceMap[handle]);
			}
			else
			{
				pass->RegisterFrameResource(handle, m_allocatedResourceMap[handle]);
			}
		}
	}

	// TODO：上一帧未被复用的资源将被移除 这里先放到pending，等fence完成再销毁
	for (auto& [desc, vec] : m_lastResourceUsingMap)
	{
		for (auto& res : vec)
		{
			//m_pendingDestroy.push_back({ res, m_lastSubmitFenceValueOfPrevFrame });
		}
	}

	// 生成GUI数据
	GenerateGUIData();

	// Compile结束，准备好资源分配方案，清空上次的复用记录
	m_lastResourceUsingMap = std::move(m_resourceUsingMap);
}

void NXRenderGraph::Execute()
{
	auto cQ = NXGlobalDX::GlobalCmdQueue();
	auto cA = NXGlobalDX::CurrentCmdAllocator();
	auto cL = NXGlobalDX::CurrentCmdList();

	cA->Reset();
	cL->Reset(cA, nullptr);

	std::string eventName = "NXRenderGraph";
	NX12Util::BeginEvent(cL, eventName.c_str());

	cL->OMSetRenderTargets(0, nullptr, FALSE, nullptr);

	ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
	cL->SetDescriptorHeaps(1, ppHeaps);
	cL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto pass : m_passNodes)
	{
		pass->Execute(cL);
	}

	NX12Util::EndEvent(cL);

	cL->Close();

	//NXVTStreaming->GetFenceSync().ReadBegin(cQ);

	ID3D12CommandList* pCmdLists[] = { cL };
	cQ->ExecuteCommandLists(1, pCmdLists);

	//NXVTStreaming->GetFenceSync().ReadEnd(cQ);
}

void NXRenderGraph::Clear()
{
	// 清空所有pass
	for (auto& pass : m_passNodes)
	{
		delete pass;
	}
	m_passNodes.clear();
	m_handlePassNodes.clear();

	// 清空资源映射
	for (auto& [handle, resNode] : m_resourceMap)
	{
		delete resNode;
	}
	m_resourceMap.clear();
	m_importedResourceMap.clear();
	// 清空其他辅助数据结构
	m_adjTablePassMap.clear();
	m_indegreePassMap.clear();
	m_timeLayerPassMap.clear();
	m_resourceLifeTimeMap.clear();
	m_descLifeTimesMap.clear();
	m_allocatedResourceMap.clear();
	m_resourceUsingMap.clear();
	m_allocatedHandlesMap.clear();
	m_importedHandlesMap.clear();

	// 重置RGHandle 从0计数
	NXRGHandle::Reset();
}

Ntr<NXResource> NXRenderGraph::GetResource(NXRGHandle handle)
{
	auto it = m_resourceMap.find(handle);
	if (it == m_resourceMap.end())
		return nullptr;

	if (it->second->IsImported())
	{
		auto itRes = m_importedResourceMap.find(handle);
		return itRes == m_importedResourceMap.end() ? nullptr : itRes->second;
	}
	else
	{
		auto itRes = m_allocatedResourceMap.find(handle);
		return itRes == m_allocatedResourceMap.end() ? nullptr : itRes->second;
	}
}

Ntr<NXResource> NXRenderGraph::GetUsingResourceByName(const std::string& name)
{
	for (auto [m, d] : m_lastResourceUsingMap)
	{
		for (auto& res : d)
		{
			if (res->GetName() == name)
				return res;
		}
	}

	return nullptr;
}

void NXRenderGraph::GenerateGUIData()
{
	m_guiVirtualResources.clear();
	m_guiPhysicalResources.clear();
	m_guiImportedResources.clear();
	m_maxTimeLayer = 0;
	m_minTimeLayer = std::numeric_limits<int>::max();

	// 生成虚拟资源（NXRGResource*）的GUI数据
	for (auto& [handle, lifeTime] : m_resourceLifeTimeMap)
	{
		if (lifeTime.start == std::numeric_limits<int>::max() || lifeTime.end == std::numeric_limits<int>::min())
			continue;

		auto it = m_resourceMap.find(handle);
		if (it == m_resourceMap.end()) continue;

		NXRGGUIResource res;
		res.name = it->second->GetName();
		res.handles.push_back(handle);
		res.lifeTimes.push_back(lifeTime);
		res.isImported = false;

		m_guiVirtualResources.push_back(res);
		m_minTimeLayer = std::min(m_minTimeLayer, lifeTime.start);
		m_maxTimeLayer = std::max(m_maxTimeLayer, lifeTime.end);
	}

	// 生成实际资源（Ntr<NXResource>）的GUI数据-Create部分
	for (auto& [pResource, handles] : m_allocatedHandlesMap)
	{
		NXRGGUIResource res;
		res.name = pResource->GetName();
		res.isImported = false;

		for (auto& handle : handles)
		{
			auto it = m_resourceLifeTimeMap.find(handle);
			if (it != m_resourceLifeTimeMap.end())
			{
				res.handles.push_back(it->first);
				res.lifeTimes.push_back(it->second);
			}
		}

		m_guiPhysicalResources.push_back(res);
	}

	// 生成实际资源（Ntr<NXResource>）的GUI数据-Import部分
	for (auto& [pResource, handle] : m_importedHandlesMap)
	{
		NXRGGUIResource res;
		res.name = pResource->GetName();

		for (auto& pass : m_handlePassNodes[handle])
		{
			res.handles.push_back(handle);
			res.lifeTimes.push_back({ m_timeLayerPassMap[pass], m_timeLayerPassMap[pass] + 1, nullptr, nullptr }); // import资源生命周期贯穿始终
		}

		res.isImported = true;

		m_guiImportedResources.push_back(res);
	}
}

Ntr<NXResource> NXRenderGraph::CreateResourceByDescription(const NXRGDescription& desc, NXRGHandle handle)
{
	std::string strResName = "NXRGRes_" + std::to_string(s_resourceId++);

	// 先尝试从 m_lastResourceUsingMap 复用资源...
	if (m_lastResourceUsingMap.find(desc) != m_lastResourceUsingMap.end())
	{
		if (!m_lastResourceUsingMap[desc].empty())
		{
			Ntr<NXResource> pRes = m_lastResourceUsingMap[desc].front(); // 从队头取，尽量保持顺序
			m_lastResourceUsingMap[desc].erase(m_lastResourceUsingMap[desc].begin());
			m_resourceUsingMap[desc].push_back(pRes);
			return pRes;
		}
	}

	// ...如果没法复用，就新建

	if (desc.resourceType == NXResourceType::Tex2D)
	{
		uint32_t rtvCount = 0;
		uint32_t dsvCount = 0;
		uint32_t uavCount = 0;
		uint32_t srvCount = 1;

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.usage == NXRGResourceUsage::RenderTarget)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			rtvCount++;
		}
		else if (desc.usage == NXRGResourceUsage::DepthStencil)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			dsvCount++;
		}
		else if (desc.usage == NXRGResourceUsage::UnorderedAccess)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			uavCount++;
		}
		
		Ntr<NXTexture2D> pTex(new NXTexture2D());
		pTex->CreateRenderTexture(strResName.c_str(), desc.tex.format, desc.tex.width, desc.tex.height, flags);
		pTex->SetViews(srvCount, rtvCount, dsvCount, uavCount);
		if (rtvCount) pTex->SetRTV(0);
		if (dsvCount) pTex->SetDSV(0);
		if (uavCount) pTex->SetUAV(0);
		if (srvCount) pTex->SetSRV(0);
		m_resourceUsingMap[desc].push_back(pTex);
		return pTex;
	}

	if (desc.resourceType == NXResourceType::Buffer)
	{
		Ntr<NXBuffer> pBuffer(new NXBuffer(strResName));
		pBuffer->Create(desc.buf.stride, desc.buf.arraySize);
		m_resourceUsingMap[desc].push_back(pBuffer);
		return pBuffer;
	}

	// TODO：暂不支持其他资源类型（Tex1D、Tex3D、Tex2DArray Cube）自建RT
	// （shadowMap的Tex2dArray现在是import直连外部，先凑合着用）
	assert(false);
	return nullptr;
}

const std::set<std::string>& NXRenderGraph::GetPassesAtTimeLayer(int timeLayer) const
{
	static std::set<std::string> result;
	result.clear();
	for (const auto& [pass, layer] : m_timeLayerPassMap)
	{
		if (layer == timeLayer)
		{
			result.insert(pass->GetName());
		}
	}
	return result;
}
