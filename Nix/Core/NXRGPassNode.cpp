#include "NXRGPassNode.h"
#include "NXRenderGraph.h"

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

NXRGResource* NXRGPassNodeBase::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion)
{
	// ���֮ǰû��д�������ô����Ҫ�����°汾
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, outRTIndex });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, outRTIndex });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // ��ӵ�graph��
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteDS(NXRGResource* pResource, bool useOldVersion)
{
	// ���Ҫ�������أ���֮ǰû��д�������ô����Ҫ�����°汾
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uint32_t(-1) });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uint32_t(-1) });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // ��ӵ�graph��
	return pNewVersionResource;
}

void NXRGPassNodeBase::Compile(bool isResize)
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

	if (!isResize)
		m_pPass->SetupInternal();
}
