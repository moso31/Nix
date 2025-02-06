#include "NXRenderGraph.h"
#include "NXRGHandle.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXTexture.h"
#include "NXResourceManager.h"

NXRenderGraph::NXRenderGraph()
{
}

NXRenderGraph::~NXRenderGraph()
{
}

void NXRenderGraph::Compile()
{
	// 2025.2.5 目前RenderGraph会为每个Handle Version都创建一个RT。
	for (auto pResource : m_resources)
	{
		auto& desc = pResource->GetDescription();
		if (desc.useViewResolution)
		{
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
			desc.handleFlags == RG_RenderTarget ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 0;
			desc.handleFlags == RG_DepthStencil ? flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 0;

			Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
			uint32_t width = static_cast<uint32_t>(m_viewResolution.x * desc.viewResolutionRatio);
			uint32_t height = static_cast<uint32_t>(m_viewResolution.y * desc.viewResolutionRatio);
			pTexture2D->CreateRenderTexture("", desc.format, width, height, flags);

			uint32_t rtvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;
			uint32_t dsvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? 1 : 0;
			uint32_t srvCount = 1;

			pTexture2D->SetViews(srvCount, rtvCount, dsvCount, 0);

			if (rtvCount) pTexture2D->SetRTV(0);
			if (dsvCount) pTexture2D->SetDSV(0);
			pTexture2D->SetSRV(0);

			pResource->SetResource(pTexture2D);
		}
		else
		{
			// TODO : 处理非viewResolution的情况
			// 比如将来添加了CSMDepth的tex2DArray支持，应该需要走这里
		}
	}

	for (auto pass : m_passNodes)
	{
		pass->Compile();
	}
}

void NXRenderGraph::Execute(ID3D12GraphicsCommandList* pCmdList)
{
	for (auto pass : m_passNodes)
	{
		pass->Execute(pCmdList);
	}
}

Ntr<NXTexture> NXRenderGraph::GetPresent()
{
	return m_presentResource->GetResource();
}

void NXRenderGraph::AddResource(NXRGResource* pResource)
{
	m_resources.emplace_back(pResource);
}

NXRendererPass* NXRenderGraph::GetRenderPass(const std::string& passName)
{
	auto it = std::find_if(m_passNodes.begin(), m_passNodes.end(), [&](NXRGPassNode* passNode) {
		return passNode->GetName() == passName;
	});

	if (it != m_passNodes.end())
		return (*it)->GetRenderPass();

	return nullptr;
}

NXRGPassNode* NXRenderGraph::GetPassNode(const std::string& passName)
{
	auto it = std::find_if(m_passNodes.begin(), m_passNodes.end(), [&](NXRGPassNode* passNode) {
		return passNode->GetName() == passName;
	});

	if (it != m_passNodes.end())
		return *it;

	return nullptr;
}

void NXRenderGraph::ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource)
{
	auto& pResDesc = pResource->GetDescription();
	if (pResDesc.handleFlags == RG_RenderTarget)
	{
		pCmdList->ClearRenderTargetView(pResource->GetResource()->GetRTV(), Colors::Black, 0, nullptr);
		return;
	}

	if (pResDesc.handleFlags == RG_DepthStencil)
	{
		pCmdList->ClearDepthStencilView(pResource->GetResource()->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
		return;
	}
}

void NXRenderGraph::SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size)
{
	auto vpCamera = NX12Util::ViewPort(size.x, size.y);
	pCmdList->RSSetViewports(1, &vpCamera);
	pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
}
