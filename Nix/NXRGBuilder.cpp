#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

NXRGResource* NXRGBuilder::Create(const std::string& resourceName, const NXRGDescription& desc)
{
	return m_pPassNode->Create(resourceName, desc);
}

void NXRGBuilder::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	return m_pPassNode->Read(pResource, passSlotIndex);
}

NXRGResource* NXRGBuilder::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep)
{
	return m_pPassNode->WriteRT(pResource, outRTIndex, keep);
}

NXRGResource* NXRGBuilder::WriteDS(NXRGResource* pResource, bool keep)
{
	return m_pPassNode->WriteDS(pResource, keep);
}

