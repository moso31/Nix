#include "NXSubSurfaceRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"
#include "NXResourceManager.h"
#include "NXSamplerManager.h"
#include "NXTexture.h"
#include "NXScene.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

NXSubSurfaceRenderer::NXSubSurfaceRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXSubSurfaceRenderer::~NXSubSurfaceRenderer()
{
}

void NXSubSurfaceRenderer::Init()
{
	m_pTexPassIn[0] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0);
	m_pTexPassIn[1] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting1);
	m_pTexPassIn[2] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting2);
	m_pTexPassIn[3] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	m_pTexPassIn[4] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ_R32);
	m_pTexPassIn[5] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64);
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting);
	m_pTexDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	// t0~t5, s0, b3
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0) // t0~t5
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParam = {
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(3, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP)
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParam, samplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\SSSSSRenderer.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\SSSSSRenderer.fx", "PS", pPSBlob.GetAddressOf());

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
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXSubSurfaceRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "SSSSS");
	static int RenderMode = 0;
	if (RenderMode == 0) RenderSSSSS(pCmdList);
	NX12Util::EndEvent(pCmdList);
}

void NXSubSurfaceRenderer::RenderSSSSS(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->OMSetStencilRef(0x01);

	pCmdList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);
	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle[6];
	for (int i = 0; i < _countof(srvHandle); i++)
		srvHandle[i] = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn[i]->GetSRV());

	pCmdList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbCamera.GetGPUHandle());
	pCmdList->SetGraphicsRootConstantBufferView(1, NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());
	pCmdList->SetGraphicsRootDescriptorTable(2, srvHandle[0]);

	pCmdList->OMSetStencilRef(0x00);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
	pCmdList->IASetIndexBuffer(&meshView.ibv);
	pCmdList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);
}

void NXSubSurfaceRenderer::Release()
{
}
