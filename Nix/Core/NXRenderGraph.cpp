#include "BaseDefs/DX12.h"
#include "NXRenderGraph.h"
#include "NXRGResourceVersion.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXTexture.h"
#include "NXResourceManager.h"
#include "NXGlobalDefinitions.h"
#include "NXVirtualTextureStreaming.h"

NXRenderGraph::NXRenderGraph()
{
}

NXRenderGraph::~NXRenderGraph()
{
}

void NXRenderGraph::Compile(bool isResize)
{
	for (auto pass : m_passNodes)
	{
		pass->Compile(isResize);
	}

	// 创建上下文相关（DX12 命令分配器+命令列表）
	for (int k = 0; k < m_ctx.size(); k++)
	{
		auto& ctx = m_ctx[k];
		auto& passes = m_passCtxMap[k];

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			auto& cA = ctx.cmdAllocator.Get(i);
			auto& cL = ctx.cmdList.Get(i);

			// 已经创建过就不用重新创建了
			if (cA && cL) continue;

			cA = NX12Util::CreateCommandAllocator(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
			std::wstring strCmdAllocatorName(L"NXRG CmdAlloc " + std::to_wstring(k) + L" ctx " + std::to_wstring(i));
			cA->SetName(strCmdAllocatorName.c_str());

			cL = NX12Util::CreateGraphicsCommandList(NXGlobalDX::GetDevice(), cA.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
			std::wstring strCmdListName(L"NXRG CmdList " + std::to_wstring(k) + L" ctx " + std::to_wstring(i));
			cL->SetName(strCmdListName.c_str());
		}

		// 绑定一下pass和Ctx
		for (auto pass : passes)
			pass->SetCommandContext(ctx);
	}
}

void NXRenderGraph::Execute()
{
	// 动态构建RenderGraph内部创建的Resources
	for (auto pResource : m_resources)
	{
		auto& desc = pResource->GetDescription();
		auto pRes = pResource->GetResource();

		if (desc.isImported) 
		{
			// 外部导入的不需要动态构建，设置一下SetResource，确保后续周期 GetResource() 接口能正常用就行
			pResource->SetResource(desc.importData.pImportResource);
		}
		else if (desc.type == NXResourceType::Tex2D)
		{
			if (desc.isViewRT) 
			{
				// RT的话，需要动态构建
				uint32_t rtWidth = (uint32_t)std::ceil(desc.RTScale * m_viewResolution.x);
				uint32_t rtHeight = (uint32_t)std::ceil(desc.RTScale * m_viewResolution.y);

				auto pTex = pRes.As<NXTexture2D>();
				if (pTex.IsNull() || pTex->GetWidth() != rtWidth || pTex->GetHeight() != rtHeight)
				{
					D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
					desc.handleFlags == RG_RenderTarget ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 0;
					desc.handleFlags == RG_DepthStencil ? flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 0;

					Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
					pTexture2D->CreateRenderTexture(pResource->GetName(), desc.format, rtWidth, rtHeight, flags);

					uint32_t rtvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;
					uint32_t dsvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? 1 : 0;
					uint32_t srvCount = 1;

					pTexture2D->SetViews(srvCount, rtvCount, dsvCount, 0);

					if (rtvCount) pTexture2D->SetRTV(0);
					if (dsvCount) pTexture2D->SetDSV(0);
					pTexture2D->SetSRV(0);

					pResource->SetResource(pTexture2D);
				}
			}
		}
		else if (desc.type == NXResourceType::Buffer)
		{
			// 有时候会出现需要用一维buffer 表述ViewRT的情况，比如VT随屏幕读取像素流数据
			// 这时的bufferSize是依赖屏幕分辨率的
			if (desc.isViewRT) 
			{
				// RT的话，需要动态构建
				uint32_t rtWidth = (uint32_t)std::ceil(desc.RTScale * m_viewResolution.x);
				uint32_t rtHeight = (uint32_t)std::ceil(desc.RTScale * m_viewResolution.y);

				auto pBuffer = pRes.As<NXBuffer>();
				if (pBuffer.IsNull() || pBuffer->GetWidth() != rtWidth * rtHeight)
				{
					Ntr<NXBuffer> pNewBuffer(new NXBuffer("RG Buffer"));
					pNewBuffer->Create(desc.stride, rtWidth * rtHeight);

					pResource->SetResource(pNewBuffer);
				}
			}
		}
	}

	auto cQ = NXGlobalDX::GlobalCmdQueue();

	for (auto i = 0; i < m_passCtxMap.size(); i++)
	{
		auto ctx = m_ctx[i]; // 这一组使用的ctx（DX12 CA+CL）
		auto passes = m_passCtxMap[i]; // 这一组关联的pass

		auto cA = ctx.cmdAllocator.Current().Get();
		auto cL = ctx.cmdList.Current().Get();

		cA->Reset();
		cL->Reset(cA, nullptr);

		std::string eventName = "NXRG ctx " + std::to_string(i);

		NX12Util::BeginEvent(cL, eventName.c_str());

		ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
		cL->SetDescriptorHeaps(1, ppHeaps);
		cL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (auto pass : passes)
		{
			pass->Execute(ctx.cmdList.Current().Get());
		}

		NX12Util::EndEvent(cL);

		cL->Close();

		if (i == 1)
		{
			//NXVTStreaming->GetFenceSync().ReadBegin(cQ);
		}

		ID3D12CommandList* pCmdLists[] = { cL };
		cQ->ExecuteCommandLists(1, pCmdLists);

		if (i == 1)
		{
			//NXVTStreaming->GetFenceSync().ReadEnd(cQ);
		}
	}
}

Ntr<NXTexture> NXRenderGraph::GetPresent()
{
	return m_presentResource->GetResource();
}

NXRGResource* NXRenderGraph::CreateResource(const std::string& resourceName, const NXRGDescription& desc)
{
	NXRGResource* pResource = new NXRGResource(resourceName, desc);
	m_resources.emplace_back(pResource);
	return pResource;
}

NXRGResource* NXRenderGraph::ImportTexture(const Ntr<NXTexture>& pTexture, NXRGHandleFlags flag)
{
	NXRGDescription desc;
	desc.isImported = true;
	desc.importData.pImportResource = pTexture;
	desc.importData.width = pTexture->GetWidth();
	desc.importData.height = pTexture->GetHeight();
	desc.importData.arraySize = pTexture->GetArraySize();
	desc.isViewRT = false;
	desc.type = pTexture->GetResourceType();
	desc.format = pTexture->GetFormat();
	desc.handleFlags = flag;

	NXRGResource* pResource = new NXRGResource(pTexture->GetName(), desc);
	m_resources.emplace_back(pResource);
	return pResource;
}

NXRGResource* NXRenderGraph::ImportBuffer(const Ntr<NXBuffer>& pBuffer)
{
	NXRGDescription desc;
	desc.isImported = true;
	desc.importData.pImportResource = pBuffer;
	desc.importData.width = pBuffer->GetByteSize();
	desc.importData.height = 1; 
	desc.importData.arraySize = 1;
	desc.isViewRT = false;
	desc.type = NXResourceType::Buffer;
	desc.format = DXGI_FORMAT_UNKNOWN; // Buffer没有格式 // 可能某些情况下需要R32_TYPELESS 但目前没用到
	desc.handleFlags = RG_None;

	NXRGResource* pResource = new NXRGResource(pBuffer->GetName(), desc);
	m_resources.emplace_back(pResource);
	return pResource;
}

NXRenderPass* NXRenderGraph::GetRenderPass(const std::string& passName)
{
	auto it = std::find_if(m_passNodes.begin(), m_passNodes.end(), [&](NXRGPassNodeBase* passNode) {
		return passNode->GetName() == passName;
	});

	if (it != m_passNodes.end())
		return (*it)->GetRenderPass();

	return nullptr;
}

void NXRenderGraph::ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource)
{
	if (pResource->GetResource()->GetResourceType() != NXResourceType::Tex2D)
		return;

	auto& pResDesc = pResource->GetDescription();
	Ntr<NXTexture> pTexture = pResource->GetResource();
	if (pResDesc.handleFlags == RG_RenderTarget)
	{
		pTexture->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET); // dx12 需要及时更新资源状态
		pCmdList->ClearRenderTargetView(pTexture->GetRTV(), Colors::Black, 0, nullptr);
		return;
	}

	if (pResDesc.handleFlags == RG_DepthStencil)
	{
		pTexture->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE); // dx12 需要及时更新资源状态
		pCmdList->ClearDepthStencilView(pTexture->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
		return;
	}
}

void NXRenderGraph::SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size)
{
	auto vpCamera = NX12Util::ViewPort(size.x, size.y);
	pCmdList->RSSetViewports(1, &vpCamera);
	pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
}

void NXRenderGraph::Destroy()
{
	for (auto& pResource : m_resources) delete pResource;
	m_resources.clear();
	for (auto& passNode : m_passNodes) delete passNode;
	m_passNodes.clear();
}

void NXRenderGraph::SetCommandContextGroup(uint32_t index, NXRGPassNodeBase* pPass)
{
	if (m_ctx.size() < index + 1)
	{
		m_ctx.resize(index + 1);
		m_passCtxMap.resize(index + 1);
	}

	m_passCtxMap[index].push_back(pPass);
}
