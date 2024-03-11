#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"

#include "NXColorMappingRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXResourceManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"
#include "NXSamplerManager.h"

NXColorMappingRenderer::NXColorMappingRenderer() :
	m_bEnablePostProcessing(true)
{
}

NXColorMappingRenderer::~NXColorMappingRenderer()
{
}

void NXColorMappingRenderer::Init()
{
	m_pTexPassIn = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting);
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\ColorMapping.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ColorMapping.fx", "PS", pPSBlob.Get());

	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParams = {
		NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers = { 
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL) 
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

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
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXCBufferAllocator->Alloc(ResourceType_Upload, m_cbParams.Get(i));

	}
}

void NXColorMappingRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Post Processing");

	m_cbParams.Current().param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
	NXCBufferAllocator->UpdateData(m_cbParams.Current());

	NX12Util::BeginEvent(m_pCommandList.Get(), "Color Mapping");

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	auto srvHandle = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn->GetSRVArray(), m_pTexPassIn->GetSRVs());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->OMSetRenderTargets(0, &m_pTexPassOut->GetRTV(), false, nullptr);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	m_pCommandList->SetGraphicsRootConstantBufferView(0, m_cbParams.GetGPUHandle());
	m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);
	
	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	NX12Util::EndEvent(); 
	NX12Util::EndEvent();
}

void NXColorMappingRenderer::Release()
{
}
