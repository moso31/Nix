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

NXRGResource* NXRGPassNodeBase::Write(NXRGResource* pResource, uint32_t outRTIndex)
{
	if (!pResource->HasWrited())
	{
		// ���֮ǰû��д���������ֱ����
		m_outputs.push_back({ pResource, outRTIndex });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}
	else
	{
		// �����°汾
		NXRGResource* pNewVersionResource = new NXRGResource(pResource);
		m_outputs.push_back({ pNewVersionResource, outRTIndex });
		pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
		m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
		return pNewVersionResource;
	}
}

void NXRGPassNodeBase::Compile()
{
	for (auto pInResSlot : m_inputs)
	{
		m_pPass->SetInputTex(pInResSlot.resource->GetResource(), pInResSlot.slot);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		auto flag = pOutRes->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			m_pPass->SetOutputRT(pOutRes->GetResource(), pOutResSlot.slot);
		}
		else if (flag == RG_DepthStencil)
		{
			m_pPass->SetOutputDS(pOutRes->GetResource());
		}
	}

	m_pPass->SetupInternal();
}
