#include "NXRenderGraph.h"
#include "NXRGHandle.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXTexture.h"

NXRenderGraph::NXRenderGraph()
{
}

NXRenderGraph::~NXRenderGraph()
{
}

void NXRenderGraph::AddPass(NXRGPassNode* pPassNode, std::function<void()> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList)> execute)
{
	pPassNode->RegisterSetupFunc(setup);
	pPassNode->RegisterExecuteFunc(execute);
	m_passNodes.push_back(pPassNode);
}

void NXRenderGraph::Compile()
{
	for (auto pResource : m_resources)
	{
		auto& desc = pResource->GetDescription();
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		desc.handleFlags == RG_RenderTarget ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 0;
		desc.handleFlags == RG_DepthStencil ? flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 0;

		Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
		pTexture2D->CreateRenderTexture("", desc.format, desc.width, desc.height, flags);

		uint32_t rtvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;
		uint32_t dsvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? 1 : 0;
		uint32_t srvCount = 1;

		pTexture2D->SetViews(srvCount, rtvCount, dsvCount, 0);

		if (rtvCount) pTexture2D->SetRTV(0);
		if (dsvCount) pTexture2D->SetDSV(0);
		pTexture2D->SetSRV(0);

		pResource->SetResource(pTexture2D);
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

void NXRenderGraph::AddResource(NXRGResource* pResource)
{
	m_resources.emplace_back(pResource);
}
