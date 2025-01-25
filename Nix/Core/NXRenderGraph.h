#pragma once
#include "NXRendererPass.h"

class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	void AddPass(NXRendererPass* pPass);
	void Sort();
	void Execute();

private:
	std::vector<NXRendererPass*> m_Passes;
};
