#pragma once
#include "NXRenderGraph.h"
#include "NXRGResource.h"

class NXRendererPass;
class NXRGPassNode
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName, NXRendererPass* pPass) : 
		m_pRenderGraph(pRenderGraph), m_passName(passName), m_pPass(pPass) {}

	const std::string& GetName() { return m_passName; }

	// 声明一个RenderGraph使用的资源。
	NXRGResource* Create(const NXRGDescription& desc);

	// 设置Pass输入资源。
	void Read(NXRGResource* pResource);

	// 设置pass输出RT。
	NXRGResource* Write(NXRGResource* pResource);

	NXRendererPass* GetRenderPass() { return m_pPass; }

	void Compile();
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	void RegisterSetupFunc(std::function<void()> func) { m_setupFunc = func; }
	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList)> func) { m_executeFunc = func; }

private:
	std::string m_passName;
	NXRenderGraph* m_pRenderGraph;
	NXRendererPass* m_pPass;

	std::vector<NXRGResource*> m_inputs;
	std::vector<NXRGResource*> m_outputs;

	// 如果需要设置pass的各种附加信息（比如gbuffer依赖的camera），可使用此方法的lambda。
	std::function<void()> m_setupFunc;

	// 如果需要在执行前进行一些pass操作（比如clearRT），可使用此方法的lambda。
	std::function<void(ID3D12GraphicsCommandList* pCmdList)> m_executeFunc;
};
