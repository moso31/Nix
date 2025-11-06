#include "NXRenderGraph.h"
#include "NXTexture.h"
#include "NXBuffer.h"

NXRGHandle NXRenderGraph::Read(NXRGHandle resID, NXRGPassNodeBase* passNode)
{
	passNode->AddInput(resID);
	return resID;
}

NXRGHandle NXRenderGraph::Write(NXRGPassNodeBase* passNode, NXRGHandle resID)
{
	NXRGResource* pResource = new NXRGResource(m_resourceMap[resID]);
	NXRGHandle handle = pResource->GetHandle();

	passNode->AddOutput(handle);
	m_resourceNodes.push_back(pResource);
	m_resourceMap[handle] = pResource;
	return handle;
}

NXRGHandle NXRenderGraph::Create(const std::string& name, const NXRGDescription& desc)
{
	NXRGResource* pResource = new NXRGResource(name, desc);
	NXRGHandle handle = pResource->GetHandle();

	m_resourceNodes.push_back(pResource);
	m_resourceMap[handle] = pResource;
	return handle;
}

NXRGHandle NXRenderGraph::Import(const Ntr<NXResource>& importResource)
{
	NXRGDescription desc;
	NXRGResource* pResource = new NXRGResource(importResource->GetName(), desc);
	m_externalNodes.push_back(pResource);
}
