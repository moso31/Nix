#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"

#include "NXRGBuilder.h"
#include "NXRGPassNode.h"
#include "NXGraphicPass.h"
#include "NXComputePass.h"

class NXRenderGraph
{
public:
	NXRenderGraph();
	virtual ~NXRenderGraph();

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddPass(const std::string& name, NXGraphicPass* pRendererPass, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pRendererPass);
		NXRGBuilder pBuilder(this, pPassNode);
		setup(pBuilder, pPassNode->GetData());

		pPassNode->RegisterExecuteFunc(execute);
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	template<typename NXRGPassData>
	NXRGPassNode<NXRGPassData>* AddComputePass(const std::string& name, NXComputePass* pComputePass, std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup, std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> execute)
	{
		auto pPassNode = new NXRGPassNode<NXRGPassData>(this, name, pComputePass);
		NXRGBuilder pBuilder(this, pPassNode);
		setup(pBuilder, pPassNode->GetData());

		pPassNode->RegisterExecuteFunc(execute);
		m_passNodes.push_back(pPassNode);
		return pPassNode;
	}

	void Compile(bool isResize = false);
	void Execute(ID3D12GraphicsCommandList* pCmdList);

	Ntr<NXTexture> GetPresent();
	void SetPresent(NXRGResource* pResource) { m_presentResource = pResource; }

	NXRGResource* CreateResource(const std::string& resourceName, const NXRGDescription& desc);
	NXRGResource* ImportTexture(const Ntr<NXTexture>& pTexture, NXRGHandleFlags flag = RG_None);
	NXRGResource* ImportBuffer(const Ntr<NXBuffer>& pBuffer);
	void SetViewResolution(const Vector2& resolution) { m_viewResolution = resolution; }	

	NXRenderPass* GetRenderPass(const std::string& passName);

	// ��ȡ��Դ��pass�Ľӿ�
	const std::vector<NXRGResource*>& GetResources() { return m_resources; }
	const std::vector<NXRGPassNodeBase*>& GetPassNodes() { return m_passNodes; }

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
