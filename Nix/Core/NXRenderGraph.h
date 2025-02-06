#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"

class NXRGHandle;
class NXRGPassNode;
class NXRGResource;
struct NXRGDescription;
class NXRendererPass;
class NXTexture;
class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	template<typename PassData>
	void AddPass(NXRGPassNode* pPassNode, PassData& passData, std::function<void()> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList)> execute)
	{
		pPassNode->RegisterSetupFunc(setup);
		pPassNode->RegisterExecuteFunc(execute);
		m_passNodes.push_back(pPassNode);
	}

	void Compile();
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	Ntr<NXTexture> GetPresent();
	void SetPresent(NXRGResource* pResource) { m_presentResource = pResource; }

	void AddResource(NXRGResource* pResources);
	void SetViewResolution(const Vector2& resolution) { m_viewResolution = resolution; }

	NXRendererPass* GetRenderPass(const std::string& passName);
	NXRGPassNode* GetPassNode(const std::string& passName);

	// ����dx��API���RT��ֻ���� Execute() lambda �е��á�
	void ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource);
	void SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size);

private:
	// ͼ����������pass
	std::vector<NXRGPassNode*> m_passNodes;

	// ͼ��������ԴRT
	std::vector<NXRGResource*> m_resources;

	// ���ճ���ʹ�õ�RT
	NXRGResource* m_presentResource;

	Vector2 m_viewResolution;
};
