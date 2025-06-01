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
	// 2025.2.5 目前RenderGraph会为每个Handle Version都创建一个RT。
	for (auto pResource : m_resources)
	{
		auto& desc = pResource->GetDescription();
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		desc.handleFlags == RG_RenderTarget ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 0;
		desc.handleFlags == RG_DepthStencil ? flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 0;

		// 如果是viewRT（跟随view变化），使用动态分辨率和动态缩放比例；否则使用desc的分辨率
		uint32_t width = static_cast<uint32_t>(desc.isViewRT ? m_viewResolution.x * desc.RTScale : desc.importData.width);
		uint32_t height = static_cast<uint32_t>(desc.isViewRT ? m_viewResolution.y * desc.RTScale : desc.importData.height);

		if (!desc.isImported) // 如果非导入，需要自己创建纹理
		{
			if (desc.type == NXResourceType::Tex2D)
			{
				Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
				pTexture2D->CreateRenderTexture(pResource->GetName(), desc.format, width, height, flags);

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
				// TODO：其他的还没用上，懒得写 // 而且要铺开搞的话 是不是走ResourceManager好一些？
			}
		}
		else
		{
			// 外部导入的不需要创建
			pResource->SetResource(desc.importData.pImportResource);
		}
	}

	for (auto pass : m_passNodes)
	{
		pass->Compile(isResize);
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
