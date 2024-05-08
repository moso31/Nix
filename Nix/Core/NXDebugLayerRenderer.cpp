#include "NXDebugLayerRenderer.h"
#include "NXShadowMapRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXGlobalDefinitions.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"

NXDebugLayerRenderer::NXDebugLayerRenderer(NXShadowMapRenderer* pShadowMapRenderer) :
	m_pShadowMapRenderer(pShadowMapRenderer),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
}

void NXDebugLayerRenderer::Init(const Vector2& rtSize)
{
	m_pTexPassIn0 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);
	m_pTexPassIn1 = m_pShadowMapRenderer->GetShadowMapDepthTex();

	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Debug Layer Out RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTexPassOut->AddRTV();
	m_pTexPassOut->AddSRV();

	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0)); // t0~t1. 2 descriptors.

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.push_back(NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL)); // b2
	rootParams.push_back(NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL));

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	staticSamplers.push_back(NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP));

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\DebugLayer.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DebugLayer.fx", "PS", pPSBlob.GetAddressOf());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPassOut->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	m_cbParams.Create(NXCBufferAllocator, NXDescriptorAllocator, true);
}

void NXDebugLayerRenderer::OnResize(const Vector2& rtSize)
{
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Debug Layer Out RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTexPassOut->AddRTV();
	m_pTexPassOut->AddSRV();

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		m_cbParams.Get(i).RTSize = Vector4(rtSize.x, rtSize.y, 1.0f / rtSize.x, 1.0f / rtSize.y);
		m_cbParams.Get(i).LayerParam0 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	}
}

void NXDebugLayerRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bEnableDebugLayer)
		return;

	NX12Util::BeginEvent(pCmdList, "Debug Layer");

	// Update LayerParams
	m_cbParams.Current().LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
	m_cbParams.Current().LayerParam0.y = m_fShadowMapZoomScale;
	m_cbParams.UpdateBuffer();

	auto srvHandle0 = NXGPUHandleHeap->SetFluidDescriptor(m_pTexPassIn0->GetSRV());
	NXGPUHandleHeap->SetFluidDescriptor(m_pTexPassIn1->GetSRV());

	pCmdList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);
	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	pCmdList->SetGraphicsRootConstantBufferView(0, m_cbParams.GetGPUHandle());
	pCmdList->SetGraphicsRootDescriptorTable(1, srvHandle0);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
	pCmdList->IASetIndexBuffer(&meshView.ibv);
	pCmdList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	NX12Util::EndEvent(pCmdList);
}

void NXDebugLayerRenderer::Release()
{
}
