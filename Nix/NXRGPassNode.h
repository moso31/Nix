#pragma once
#include "NXRendererPass.h"
#include "NXRGResource.h"

struct NXRGResourceSlot
{
	NXRGResource* resource;
	uint32_t slot;
};

class NXRenderGraph;
class NXRGPassNodeBase
{
public:
	NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRendererPass* pPass) :
		m_pRenderGraph(pRenderGraph), m_passName(passName), m_pPass(pPass) {}

	const std::string& GetName() { return m_passName; }

	// ����һ��RenderGraphʹ�õ���Դ��
	NXRGResource* Create(const std::string& resourceName, const NXRGDescription& desc);

	// ����Pass������Դ��
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// ����pass���RT��
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keepPixel);
	NXRGResource* WriteDS(NXRGResource* pResource, bool keepPixel);

	NXRendererPass* GetRenderPass() { return m_pPass; }

	void Compile();
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

protected:
	std::string m_passName;
	NXRenderGraph* m_pRenderGraph;
	NXRendererPass* m_pPass;

	// Pass��¼�Լ���������Դ��������������NXRenderGraph*����
	std::vector<NXRGResourceSlot> m_inputs; 
	std::vector<NXRGResourceSlot> m_outputs;
};

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName, NXRendererPass* pPass) : NXRGPassNodeBase(pRenderGraph, passName, pPass), m_passData(NXRGPassData()) {}

	NXRGPassData& GetData() { return m_passData; }

	void Execute(ID3D12GraphicsCommandList* pCmdList) override
	{
		m_executeFunc(pCmdList, m_passData);
		m_pPass->Render(pCmdList);
	}

	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> func) { m_executeFunc = func; }

private:
	NXRGPassData m_passData;

	// �����Ҫ��ִ��ǰ����һЩpass����������clearRT������ʹ�ô˷�����lambda��
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
