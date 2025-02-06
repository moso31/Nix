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

	// 调用dx层API清空RT。只能在 Execute() lambda 中调用。
	void ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource);
	void SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size);

private:
	// 图依赖的所有pass
	std::vector<NXRGPassNode*> m_passNodes;

	// 图依赖的资源RT
	std::vector<NXRGResource*> m_resources;

	// 最终呈现使用的RT
	NXRGResource* m_presentResource;

	Vector2 m_viewResolution;
};
