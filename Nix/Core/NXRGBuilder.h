#pragma once
#include "BaseDefs/NixCore.h"
#include "NXRGUtil.h"

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

	NXRGHandle Read(NXRGHandle resID);
	NXRGHandle Write(NXRGHandle resID);
	NXRGHandle ReadWrite(NXRGHandle resID);

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
