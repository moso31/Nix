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
	void Read(NXRGResource* pResource);

	// 设置pass输出RT。
	NXRGResource* Write(NXRGResource* pResource);

	NXRendererPass* GetRenderPass() { return m_pPass; }

	void Compile();
	void Execute();

	void RegisterSetupFunc(std::function<void()> func) { m_setupFunc = func; }
	void RegisterExecuteFunc(std::function<void()> func) { m_executeFunc = func; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRendererPass* m_pPass;

	std::vector<NXRGResource*> m_inputs;
	std::vector<NXRGResource*> m_outputs;

	std::function<void()> m_setupFunc;
	std::function<void()> m_executeFunc;
};
