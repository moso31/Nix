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

	// ����һ��RenderGraphʹ�õ���Դ��
	NXRGResource* Create(const NXRGDescription& desc);

	// ����Pass������Դ��
	void Read(NXRGResource* pResource);

	// ����pass���RT��
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

	// �����Ҫ����pass�ĸ��ָ�����Ϣ������gbuffer������camera������ʹ�ô˷�����lambda��
	std::function<void()> m_setupFunc;

	// �����Ҫ��ִ��ǰ����һЩpass����������clearRT������ʹ�ô˷�����lambda��
	std::function<void(ID3D12GraphicsCommandList* pCmdList)> m_executeFunc;
};
