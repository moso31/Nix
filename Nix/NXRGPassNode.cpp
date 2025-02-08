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
		// ���֮ǰû��д���������ֱ����
		m_outputs.push_back(pResource);
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}
	else
	{
		// �����°汾
		NXRGResource* pNewVersionResource = new NXRGResource(pResource);
		m_outputs.push_back(pNewVersionResource);
		pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
		m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
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
