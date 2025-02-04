#pragma once
#include "NXRenderGraph.h"
#include "NXRGResource.h"

class NXRendererPass;
class NXRGPassNode
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, NXRendererPass* pPass) : m_pRenderGraph(pRenderGraph), m_pPass(pPass) {}

	// 声明一个RenderGraph使用的资源。
	NXRGResource* Create(const NXRGDescription& desc);

	// 设置Pass输入资源。
	NXRGResource* Read(NXRGResource* pResource);

	// 设置pass输出RT。
	NXRGResource* Write(NXRGResource* pResource);

	NXRendererPass* GetRenderPass() { return m_pPass; }

	void Execute();

private:
	NXRenderGraph* m_pRenderGraph;
	NXRendererPass* m_pPass;

	std::vector<NXRGResource*> m_inputs;
	std::vector<NXRGResource*> m_outputs;
};
