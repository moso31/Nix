#include "NXShadowTestRenderer.h"
#include "NXGlobalDefinitions.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXResourceManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXTexture.h"

NXShadowTestRenderer::~NXShadowTestRenderer()
{
}

void NXShadowTestRenderer::Init()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\ShadowTest.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowTest.fx", "PS", pPSBlob.Get());

	// t0, t1, b0, b1, b2, s0
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParams = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL), // b0
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL), // b1
		NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL), // b2
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL) // t0, t1
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER) // s0
	};

	// Create the root signature with the new configuration
	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::device.Get(), rootParams, staticSamplers);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create();;
	psoDesc.DepthStencilState = NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = m_pTexPassOut->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // shadowtest 不需要 dsv
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	m_pTexPassIn0 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	// m_pTexPassIn1 由 SetShadowMapDepth() 方法传入

	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_ShadowTest);
}

void NXShadowTestRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Shadow Test");

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	auto srvHandle0 = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn0->GetSRV());
	auto srvHandle1 = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn1->GetSRV());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	m_pCommandList->ClearRenderTargetView(m_pTexPassOut->GetRTV(), Colors::Black, 0, nullptr);
	m_pCommandList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);

	m_pCommandList->SetGraphicsRootConstantBufferView(1, NXGlobalBuffer::cbCamera.Current().GPUVirtualAddr);
	m_pCommandList->SetGraphicsRootConstantBufferView(2, NXGlobalBuffer::cbShadowTest.Current().GPUVirtualAddr);
	m_pCommandList->SetGraphicsRootDescriptorTable(0, srvHandle0);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle1);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	NX12Util::EndEvent();
}

void NXShadowTestRenderer::Release()
{
}
