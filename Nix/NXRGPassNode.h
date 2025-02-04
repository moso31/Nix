#pragma once
#include "NXRenderGraph.h"
#include "NXRGResource.h"

class NXRendererPass;
class NXRGPassNode
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, NXRendererPass* pPass) : m_pRenderGraph(pRenderGraph), m_pPass(pPass) {}

	// ����һ��RenderGraphʹ�õ���Դ��
	NXRGResource* Create(const NXRGDescription& desc);

	// ����Pass������Դ��
	NXRGResource* Read(NXRGResource* pResource);

	// ����pass���RT��
	NXRGResource* Write(NXRGResource* pResource);

	NXRendererPass* GetRenderPass() { return m_pPass; }

	void Execute();

private:
	NXRenderGraph* m_pRenderGraph;
	NXRendererPass* m_pPass;

	std::vector<NXRGResource*> m_inputs;
	std::vector<NXRGResource*> m_outputs;
};
