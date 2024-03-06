#include "NXDepthRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"
#include "NXResourceManager.h"
#include "NXSamplerManager.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

void NXDepthRenderer::Init()
{
	m_pTexPassIn = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ_R32);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\Depth.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Depth.fx", "PS", pPSBlob.Get());

	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParam = {
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP)
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParam, samplers);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS, true, 0xFF, 0xFF, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL>::Create();
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
}

void NXDepthRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Depth Copy");

	m_pCommandList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	auto srvHandle = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn->GetSRV());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->OMSetStencilRef(0x01);
	m_pCommandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	m_pCommandList->OMSetStencilRef(0x00);

	NX12Util::EndEvent();
}

void NXDepthRenderer::Release()
{
}
