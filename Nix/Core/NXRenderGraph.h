#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"

#include "NXRGBuilder.h"
#include "NXRGPassNode.h"
#include "NXRendererPass.h"

class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddPass(const std::string& name, NXRendererPass* pRendererPass, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pRendererPass);
		NXRGBuilder pBuilder(this, pPassNode);
		setup(pBuilder, pPassNode->GetData());

		pPassNode->RegisterExecuteFunc(execute);
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	void Compile();
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	Ntr<NXTexture> GetPresent();
	void SetPresent(NXRGResource* pResource) { m_presentResource = pResource; }

	void AddResource(NXRGResource* pResources);
	void SetViewResolution(const Vector2& resolution) { m_viewResolution = resolution; }

	NXRendererPass* GetRenderPass(const std::string& passName);

	// ����dx��API���RT��ֻ���� Execute() lambda �е��á�
	void ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource);
	void SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size);

private:
	// ͼ����������pass
	std::vector<NXRGPassNodeBase*> m_passNodes;

	// ͼ��������ԴRT
	std::vector<NXRGResource*> m_resources;

	// ���ճ���ʹ�õ�RT
	NXRGResource* m_presentResource;

	Vector2 m_viewResolution;
};
