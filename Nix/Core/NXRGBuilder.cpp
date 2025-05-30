#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

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

