#include "NXRGBuilder.h"
#include "NXRGPassNode.h"

NXRGResource* NXRGBuilder::Create(const NXRGDescription& desc)
{
	return m_pPassNode->Create(desc);
}

void NXRGBuilder::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	return m_pPassNode->Read(pResource, passSlotIndex);
}

NXRGResource* NXRGBuilder::Write(NXRGResource* pResource)
{
	return m_pPassNode->Write(pResource);
}
