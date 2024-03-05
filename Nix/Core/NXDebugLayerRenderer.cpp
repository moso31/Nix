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

void NXDebugLayerRenderer::Init()
{
	m_pTexPassIn0 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);
	m_pTexPassIn1 = m_pShadowMapRenderer->GetShadowMapDepthTex();

	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0)); // t0~t1. 2 descriptors.

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.push_back(NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL)); // b2
	rootParams.push_back(NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL));

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	staticSamplers.push_back(NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP));

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::device.Get(), rootParams, staticSamplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\DebugLayer.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DebugLayer.fx", "PS", pPSBlob.Get());

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
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbParams.Get(i));
}

void NXDebugLayerRenderer::OnResize(const Vector2& rtSize)
{
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Debug Layer Out RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTexPassOut->AddRTV();
	m_pTexPassOut->AddSRV();

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		m_cbParams.Get(i).data.RTSize = Vector4(rtSize.x, rtSize.y, 1.0f / rtSize.x, 1.0f / rtSize.y);
		m_cbParams.Get(i).data.LayerParam0 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	}
}

void NXDebugLayerRenderer::Render()
{
	if (!m_bEnableDebugLayer)
		return;

	NX12Util::BeginEvent(m_pCommandList.Get(), "Debug Layer");

	// Update LayerParams
	m_cbParams.Current().data.LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
	m_cbParams.Current().data.LayerParam0.y = m_fShadowMapZoomScale;
	NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(m_cbParams.Current());

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	auto srvHandle0 = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn0->GetSRV());
	auto srvHandle1 = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn1->GetSRV());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	m_pCommandList->SetGraphicsRootConstantBufferView(0, m_cbParams.Current().GPUVirtualAddr);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle0);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	NX12Util::EndEvent();
}

void NXDebugLayerRenderer::Release()
{
}
