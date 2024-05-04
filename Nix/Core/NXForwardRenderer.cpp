#include "NXForwardRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"

#include "NXBRDFlut.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

NXForwardRenderer::NXForwardRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut) :
	m_pBRDFLut(pBRDFLut),
	m_pScene(pScene)
{
}

NXForwardRenderer::~NXForwardRenderer()
{
}

void NXForwardRenderer::Init()
{
	m_pTexPassIn[0] = m_pScene->GetCubeMap()->GetCubeMap();
	//m_pTexPassIn[1] = Texture2D txAlbedo : register(t1);
	//m_pTexPassIn[2] = Texture2D txNormalMap : register(t2);
	//m_pTexPassIn[3] = Texture2D txMetallicMap : register(t3);
	//m_pTexPassIn[4] = Texture2D txRoughnessMap : register(t4);
	//m_pTexPassIn[5] = Texture2D txAmbientOcclusionMap : register(t5);
	m_pTexPassIn[6] = m_pScene->GetCubeMap()->GetPreFilterMap();
	m_pTexPassIn[7] = m_pBRDFLut->GetTex();
	//m_pTexPassIn[8] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(); // depthPeeling

	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0);
	m_pTexDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\ForwardTranslucent.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ForwardTranslucent.fx", "PS", pPSBlob.GetAddressOf());

	// t0~t8, s0~s2, b0~b5
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 0, 0)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParam = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(3, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(4, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(5, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(6, 0, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NXSamplerManager::GetInstance()->CreateIso(1, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
		NXSamplerManager::GetInstance()->CreateIso(2, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParam, samplers);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPNTT;
	psoDesc.BlendState = NXBlendState<false, false, true, false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
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

void NXForwardRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Forward rendering");
	m_pCommandList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, &m_pTexDepth->GetDSV());
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	//TODO: 回头再修这块，需要先跑通其他内容

	//g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	//g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	//g_pContext->RSSetState(m_pRasterizerState.Get());

	//auto pRTVScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0)->GetRTV();
	//auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	//g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	//g_pContext->IASetInputLayout(m_pInputLayout.Get());

	//g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	//g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	//auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
	//g_pContext->PSSetSamplers(0, 1, &pSampler);
	//pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	//g_pContext->PSSetSamplers(1, 1, &pSampler);

	//g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBuffer::cbCamera.GetAddressOf());
	//g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBuffer::cbCamera.GetAddressOf());

	//auto pCbLights = m_pScene->GetConstantBufferLights();
	//auto pCubeMap = m_pScene->GetCubeMap();
	//if (pCbLights)
	//	g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	//if (pCubeMap)
	//{
	//	auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
	//	auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
	//	auto pBRDF2DLUT = m_pBRDFLut->GetSRV();
	//	g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);
	//	g_pContext->PSSetShaderResources(8, 1, &pPreFilterMapSRV);
	//	g_pContext->PSSetShaderResources(9, 1, &pBRDF2DLUT);

	//	auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
	//	g_pContext->PSSetConstantBuffers(5, 1, &pCBCubeMapParam);
	//}

	//// PBR大改。阴影贴图暂时停用。
	////auto pShadowMapSRV = m_pPassShadowMap->GetSRV();
	////g_pContext->PSSetShaderResources(10, 1, &pShadowMapSRV);

	////auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
	////g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);

	//// 2022.4.14 只渲染 Transparent 物体
	//for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	//{
	//	// TODO
	//}

	NX12Util::EndEvent(m_pCommandList.Get());
}
