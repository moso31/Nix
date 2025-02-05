#pragma once
#include "BaseDefs/NixCore.h"

class NXRGHandle;
class NXRGPassNode;
class NXRGResource;
class NXRGDescription;
class NXTexture;
class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	void AddPass(NXRGPassNode* pPassNode, std::function<void()> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList)> execute);
	void Compile();
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	void AddResource(NXRGResource* pResources);

private:
	// 图依赖的所有pass
	std::vector<NXRGPassNode*> m_passNodes;

	// 图依赖的资源RT
	std::vector<NXRGResource*> m_resources;
};
