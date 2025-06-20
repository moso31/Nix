#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

void NXRGBuilder::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	return m_pPassNode->Read(pResource, passSlotIndex);
}

void NXRGBuilder::SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount)
{
	m_pPassNode->SetRootParamLayout(cbvCount, srvCount, uavCount);
}

void NXRGBuilder::ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer)
{
	m_pPassNode->ReadConstantBuffer(rootIndex, slotIndex, pConstantBuffer);
}

NXRGResource* NXRGBuilder::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep)
{
	return m_pPassNode->WriteRT(pResource, outRTIndex, keep);
}

NXRGResource* NXRGBuilder::WriteDS(NXRGResource* pResource, bool keep)
{
	return m_pPassNode->WriteDS(pResource, keep);
}

NXRGResource* NXRGBuilder::WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool keep, uint32_t uavCounterIndex)
{
	return m_pPassNode->WriteUAV(pResource, uavIndex, keep, uavCounterIndex);
}

NXRGResource* NXRGBuilder::SetIndirectArgs(NXRGResource* pResource)
{
	return m_pPassNode->SetIndirectArgs(pResource);
}

void NXRGBuilder::SetComputeThreadGroup(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (m_pPassNode->GetRenderPass()->GetPassType() == NXRenderPassType::ComputePass)
	{
		auto pComputePass = (NXComputePass*)m_pPassNode->GetRenderPass();
		pComputePass->SetThreadGroups(threadGroupX, threadGroupY, threadGroupZ);
	}
}

void NXRGBuilder::SetEntryNameVS(const std::wstring& entryName)
{
	m_pPassNode->GetRenderPass()->SetEntryNameVS(entryName);
}

void NXRGBuilder::SetEntryNamePS(const std::wstring& entryName)
{
	m_pPassNode->GetRenderPass()->SetEntryNamePS(entryName);
}

void NXRGBuilder::SetEntryNameCS(const std::wstring& entryName)
{
	m_pPassNode->GetRenderPass()->SetEntryNameCS(entryName);
}

