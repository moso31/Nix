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

void NXRenderGraph::Compile(bool isResize)
{
	for (auto pass : m_passNodes)
	{
		pass->Compile(isResize);
	}
}

void NXRenderGraph::Execute(ID3D12GraphicsCommandList* pCmdList)
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
			auto pTex = pRes.As<NXTexture2D>();

			if (desc.isViewRT) 
			{
				// RT的话，需要动态构建
				uint32_t rtWidth = (uint32_t)(desc.RTScale * m_viewResolution.x);
				uint32_t rtHeight = (uint32_t)(desc.RTScale * m_viewResolution.y);
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
			else 
			{
				// 非RT+非导入资源 的动态构建
				// TODO 暂时没用到这种情况，先偷懒了
			}
		}
	}

	for (auto pass : m_passNodes)
	{
		pass->Execute(pCmdList);
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
