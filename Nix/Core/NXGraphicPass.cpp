#include "NXGraphicPass.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXTexture.h"
#include "NXRenderStates.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXRGResource.h"

NXGraphicPass::NXGraphicPass() :
	NXRenderPass(NXRenderPassType::GraphicPass),
	m_psoDesc({}),
	m_stencilRef(0x0),
	m_rtSubMeshName("_RenderTarget"),
	m_pOutDS(nullptr)
{
	m_psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	m_psoDesc.BlendState = NXBlendState<>::Create();
	m_psoDesc.RasterizerState = NXRasterizerState<>::Create();
	m_psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.SampleDesc.Quality = 0;
	m_psoDesc.SampleMask = UINT_MAX;
	m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

void NXGraphicPass::SetInputTex(NXRGResource* pTex, uint32_t slotIndex)
{
	if (m_pInTexs.size() <= slotIndex) m_pInTexs.resize(slotIndex + 1);
	m_pInTexs[slotIndex] = pTex;
}

void NXGraphicPass::SetOutputRT(NXRGResource* pTex, uint32_t rtIndex)
{
	if (m_pOutRTs.size() <= rtIndex) m_pOutRTs.resize(rtIndex + 1);
	m_pOutRTs[rtIndex] = pTex;
}

void NXGraphicPass::SetOutputDS(NXRGResource* pTex)
{
	m_pOutDS = pTex;
}

void NXGraphicPass::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc)
{
	m_psoDesc.InputLayout = desc;
}

void NXGraphicPass::SetRenderTargetMesh(const std::string& rtSubMeshName)
{
	m_rtSubMeshName = rtSubMeshName;
}

void NXGraphicPass::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	m_psoDesc.BlendState = desc;
}

void NXGraphicPass::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	m_psoDesc.RasterizerState = desc;
}

void NXGraphicPass::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	m_psoDesc.DepthStencilState = desc;
}

void NXGraphicPass::SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask)
{
	m_psoDesc.SampleDesc.Count = Count;
	m_psoDesc.SampleDesc.Quality = Quality;
	m_psoDesc.SampleMask = Mask;
}

void NXGraphicPass::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	m_psoDesc.PrimitiveTopologyType = type;
}

void NXGraphicPass::InitPSO()
{
	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);

	ComPtr<IDxcBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(m_shaderFilePath, m_entryNameVS, pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(m_shaderFilePath, m_entryNamePS, pPSBlob.GetAddressOf());

	m_psoDesc.pRootSignature = m_pRootSig.Get();
	m_psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	m_psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };

	m_psoDesc.NumRenderTargets = (UINT)m_pOutRTs.size();
	for (UINT i = 0; i < m_psoDesc.NumRenderTargets; i++)
		m_psoDesc.RTVFormats[i] = m_pOutRTs[i]->GetDescription().format;
	m_psoDesc.DSVFormat = m_pOutDS ? NXConvert::TypelessToDSVFormat(m_pOutDS->GetDescription().format) : DXGI_FORMAT_UNKNOWN;

	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pPSO));

	std::wstring psoName(NXConvert::s2ws(m_passName) + L" PSO");
	m_pPSO->SetName(psoName.c_str());
}

void NXGraphicPass::RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ppRTVs;
	for (auto& pTex : m_pOutRTs)
	{
		ppRTVs.push_back(pTex->GetResource().As<NXTexture2D>()->GetRTV());
	}

	bool bHasDSV = m_pOutDS && m_pOutDS->GetResource().IsValid();
	pCmdList->OMSetRenderTargets((UINT)ppRTVs.size(), ppRTVs.data(), true, bHasDSV ? &m_pOutDS->GetResource().As<NXTexture2D>()->GetDSV() : nullptr);

	// DX12需要及时更新纹理的资源状态
	for (int i = 0; i < (int)m_pInTexs.size(); i++)
	{
		m_pInTexs[i]->GetResource()->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	for (int i = 0; i < (int)m_pOutRTs.size(); i++)
	{
		m_pOutRTs[i]->GetResource()->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	if (m_pOutDS)
	{
		m_pOutDS->GetResource()->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
}

void NXGraphicPass::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->OMSetStencilRef(m_stencilRef);

	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	for (int i = 0; i < (int)m_cbvManagements.size(); i++)
	{
		// 支持自动绑定，但自动绑定（autoUpdate）是图方便搞的语法糖，
		// 也许让上层接口自己手动更新更清晰。// 保持现状，如果未来觉得这样不合适，再改
		if (m_cbvManagements[i].autoUpdate)
		{
			const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbvManagements[i].multiFrameGpuVirtAddr->Current();
			pCmdList->SetGraphicsRootConstantBufferView(i, gpuVirtAddr);
		}
	}

	if (!m_pInTexs.empty())
	{
		for (int i = 0; i < (int)m_pInTexs.size(); i++)
		{
			auto pRes = m_pInTexs[i]->GetResource();
			NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
		}
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();

		// 2024.6.8
		// 根据目前在.h中的根参数-寄存器布局规定，
		// m_cbvManagements 中 元素的数量就是 Table 的 slot 索引。
		pCmdList->SetGraphicsRootDescriptorTable((UINT)m_cbvManagements.size(), srvHandle0);
	}
}

void NXGraphicPass::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, m_passName.c_str());

	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_rtSubMeshName);
	D3D12_VERTEX_BUFFER_VIEW vbv;
	if (meshView.GetVBV(0, vbv))
		pCmdList->IASetVertexBuffers(0, 1, &vbv);
	D3D12_INDEX_BUFFER_VIEW ibv;
	if (meshView.GetIBV(1, ibv))
		pCmdList->IASetIndexBuffer(&ibv);
	pCmdList->DrawIndexedInstanced(meshView.GetIndexCount(), 1, 0, 0, 0);

	NX12Util::EndEvent(pCmdList);
}
