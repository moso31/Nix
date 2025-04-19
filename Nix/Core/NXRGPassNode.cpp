#include "NXRGPassNode.h"
#include "NXRenderGraph.h"

NXRGResource* NXRGPassNodeBase::Create(const std::string& resourceName, const NXRGDescription & desc)
{
	NXRGResource* pResource = new NXRGResource(resourceName, desc);
	m_pRenderGraph->AddResource(pResource);
	return pResource;
}

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

NXRGResource* NXRGPassNodeBase::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keepPixel)
{
	// ���֮ǰû��д�������ô����Ҫ�����°汾
	if (keepPixel || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, outRTIndex });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, outRTIndex });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteDS(NXRGResource* pResource, bool keepPixel)
{
	// ���Ҫ�������أ���ô����Ҫ�����°汾
	// ���֮ǰû��д�������ô����Ҫ�����°汾
	if (keepPixel || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uint32_t(-1) });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uint32_t(-1) });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
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
