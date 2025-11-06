#pragma once
#include "BaseDefs/NixCore.h"
#include "NXRenderGraph.h"

class NXRenderGraph;
class NXRGPassNodeBase;
class NXRGBuilder
{
	NXRGBuilder() = delete;
	NXRGBuilder(const NXRGBuilder&) = delete;
	NXRGBuilder& operator=(const NXRGBuilder&) = delete;

public:
	NXRGBuilder(NXRenderGraph* pRenderGraph, NXRGPassNodeBase* pPassNode) :
		m_pRenderGraph(pRenderGraph), m_pPassNode(pPassNode) {}

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

	NXRGHandle Read(NXRGHandle resID) { m_pRenderGraph->Read(resID, m_pPassNode); }
	NXRGHandle Write(NXRGHandle resID) { m_pRenderGraph->Write(m_pPassNode, resID); }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
