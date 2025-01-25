#include "NXRenderGraph.h"

NXRenderGraph::NXRenderGraph()
{
}

NXRenderGraph::~NXRenderGraph()
{
}

void NXRenderGraph::AddPass(NXRendererPass* pPass)
{
	m_Passes.push_back(pPass);
}

void NXRenderGraph::Sort()
{
}

void NXRenderGraph::Execute()
{
}
