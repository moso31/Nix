#pragma once
#include "BaseDefs/NixCore.h"

class NXRGHandle;
class NXRGPassNode;
class NXRGResource;
class NXRGDescription;
class NXRendererPass;
class NXTexture;
class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	void AddPass(NXRGPassNode* pPassNode, std::function<void()> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList)> execute);
	void Compile();
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	Ntr<NXTexture> GetPresent() { return m_presentResource->GetResource(); }
	void SetPresent(NXRGResource* pResource) { m_presentResource = pResource; }

	void AddResource(NXRGResource* pResources);
	void SetViewResolution(const Vector2& resolution) { m_viewResolution = resolution; }

	NXRendererPass* GetPass(const std::string& passName);

private:
	// 图依赖的所有pass
	std::vector<NXRGPassNode*> m_passNodes;

	// 图依赖的资源RT
	std::vector<NXRGResource*> m_resources;

	// 最终呈现使用的RT
	NXRGResource* m_presentResource;

	Vector2 m_viewResolution;
};
