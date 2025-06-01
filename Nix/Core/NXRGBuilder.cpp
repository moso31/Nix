#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

void NXRGBuilder::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	return m_pPassNode->Read(pResource, passSlotIndex);
}

void NXRGBuilder::SetComputeThreadGroup(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (m_pPassNode->GetRenderPass()->GetPassType() == NXRenderPassType::ComputePass)
	{
		auto pComputePass = (NXComputePass*)m_pPassNode->GetRenderPass();
		pComputePass->SetThreadGroups(threadGroupX, threadGroupY, threadGroupZ);
	}
}

NXRGResource* NXRGBuilder::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep)
{
	return m_pPassNode->WriteRT(pResource, outRTIndex, keep);
}

NXRGResource* NXRGBuilder::WriteDS(NXRGResource* pResource, bool keep)
{
	return m_pPassNode->WriteDS(pResource, keep);
}

NXRGResource* NXRGBuilder::WriteUAV(NXRGResource* pResource, uint32_t outUAVIndex, bool keep)
{
	return m_pPassNode->WriteUAV(pResource, outUAVIndex, keep);
}

