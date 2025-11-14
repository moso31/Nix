#include "NXRGBuilder.h"
#include "NXRenderGraph.h"

NXRGHandle NXRGBuilder::Read(NXRGHandle resID) 
{
    return m_pRenderGraph->Read(resID, m_pPassNode);
}

NXRGHandle NXRGBuilder::Write(NXRGHandle resID) 
{
    return m_pRenderGraph->Write(m_pPassNode, resID);
}

NXRGHandle NXRGBuilder::ReadWrite(NXRGHandle resID)
{
	return m_pRenderGraph->ReadWrite(m_pPassNode, resID);
}
