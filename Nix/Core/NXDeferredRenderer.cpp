#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXBRDFlut.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXSubMeshGeometryEditor.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut) :
	m_pBRDFLut(pBRDFLut),
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::Init()
{
	m_pTexPassIn[0] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	m_pTexPassIn[1] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	m_pTexPassIn[2] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	m_pTexPassIn[3] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);
	m_pTexPassIn[4] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	m_pTexPassIn[5] = m_pScene->GetCubeMap()->GetCubeMap();
	m_pTexPassIn[6] = m_pScene->GetCubeMap()->GetPreFilterMap();
	m_pTexPassIn[7] = m_pBRDFLut->GetTex();
	m_pTexPassIn[8] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_ShadowTest);

	m_pTexPassOut[0] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0);
	m_pTexPassOut[1] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting1);
	m_pTexPassOut[2] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting2);
	m_pTexPassOut[3] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS("Shader\\DeferredRender.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS("Shader\\DeferredRender.fx", "PS", pPSBlob.Get());

	// t0~t8, s0~s1, b0~b4.
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 0, 0) 
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParam = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(3, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(4, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL),
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP),
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParam, samplers);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPassOut[0]->GetFormat();
	psoDesc.RTVFormats[1] = m_pTexPassOut[1]->GetFormat();
	psoDesc.RTVFormats[2] = m_pTexPassOut[2]->GetFormat();
	psoDesc.RTVFormats[3] = m_pTexPassOut[3]->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXDeferredRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Deferred rendering");

	D3D12_CPU_DESCRIPTOR_HANDLE ppRTVs[] = { m_pTexPassOut[0]->GetRTV(), m_pTexPassOut[1]->GetRTV(), m_pTexPassOut[2]->GetRTV(), m_pTexPassOut[3]->GetRTV() };
	m_pCommandList->OMSetRenderTargets(_countof(ppRTVs), ppRTVs, true, nullptr);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle[9];
	for (int i = 0; i < _countof(srvHandle); i++)
		srvHandle[i] = pShaderVisibleDescriptorHeap->Append(m_pTexPassIn[i]->GetSRV());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.GetGPUHandle());
	m_pCommandList->SetGraphicsRootConstantBufferView(1, NXGlobalBuffer::cbCamera.GetGPUHandle());
	m_pCommandList->SetGraphicsRootConstantBufferView(2, m_pScene->GetConstantBufferLights());
	m_pCommandList->SetGraphicsRootConstantBufferView(3, m_pScene->GetCubeMap()->GetCBDataParams());
	m_pCommandList->SetGraphicsRootConstantBufferView(4, NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());
	m_pCommandList->SetGraphicsRootShaderResourceView(5, srvHandle[0].ptr);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);
}

void NXDeferredRenderer::Release()
{
}
