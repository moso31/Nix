#include "NXRGPassNode.h"
#include "NXRenderGraph.h"

NXRGResource* NXRGPassNodeBase::Create(const NXRGDescription& desc)
{
	NXRGResource* pResource = new NXRGResource(desc);
	m_pRenderGraph->AddResource(pResource);
	return pResource;
}

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

NXRGResource* NXRGPassNodeBase::Write(NXRGResource* pResource)
{
	if (!pResource->HasWrited())
	{
		// 如果之前没被写入过，可以直接用
		m_outputs.push_back(pResource);
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}
	else
	{
		// 创建新版本
		NXRGResource* pNewVersionResource = new NXRGResource(pResource);
		m_outputs.push_back(pNewVersionResource);
		pNewVersionResource->MakeWriteConnect(); // 标记为已写入
		m_pRenderGraph->AddResource(pNewVersionResource); // 添加到graph中
		return pNewVersionResource;
	}
}

void NXRGPassNodeBase::Compile()
{
	for (auto pInRes : m_inputs)
	{
		m_pPass->PushInputTex(pInRes.resource->GetResource(), pInRes.slot);
	}

	for (auto pOutRes : m_outputs)
	{
		auto flag = pOutRes->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			m_pPass->PushOutputRT(pOutRes->GetResource());
		}
		else if (flag == RG_DepthStencil)
		{
			m_pPass->SetOutputDS(pOutRes->GetResource());
		}
	}

	m_pPass->SetupInternal();
}
