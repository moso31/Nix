#include "NXSkyRenderer.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXTexture.h"

#include "Global.h"
#include "NXScene.h"
#include "NXCubeMap.h"

NXSkyRenderer::NXSkyRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXSkyRenderer::~NXSkyRenderer()
{
}

void NXSkyRenderer::InitSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0));

	std::vector<D3D12_ROOT_PARAMETER> rootParam;
	rootParam.push_back(NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL));
	rootParam.push_back(NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL));
	rootParam.push_back(NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL));

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
	samplers.push_back(NXStaticSamplerState<>::Create(0, 0, D3D12_SHADER_VISIBILITY_ALL));

	NX12Util::CreateRootSignature(g_pDevice.Get(), rootParam, samplers);
}

void NXSkyRenderer::InitPSO()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\CubeMap.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMap.fx", "PS", pPSBlob.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutP, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPassOut->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXSkyRenderer::Init()
{
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting);
	m_pTexPassOutDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	NX12Util::CreateCommands(g_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.Get(), m_pCommandAllocator.Get(), m_pCommandList.Get());

	InitSignature();
	InitPSO();
}

void NXSkyRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Sky (CubeMap IBL)");
	
	auto rtvHandle = m_pTexPassOut->GetRTV();
	auto dsvHandle = m_pTexPassOutDepth->GetDSV();
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->UpdateViewParams();

		m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBufferManager::m_cbDataObject.Current().GPUVirtualAddr);
		m_pCommandList->SetGraphicsRootConstantBufferView(1, pCubeMap->GetCBDataParams());
		m_pCommandList->SetGraphicsRootDescriptorTable(2, pCubeMap->GetSRVCubeMap());

		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
		g_pContext->PSSetSamplers(0, 1, &pSampler);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(1, 1, &pCBCubeMapParam);

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);

		pCubeMap->Render();
	}

	NX12Util::EndEvent();

	/////////////////////////////////////////////////////////

	g_pUDA->BeginEvent(L"Sky (CubeMap IBL)");
	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
		g_pContext->PSSetSamplers(0, 1, &pSampler);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(1, 1, &pCBCubeMapParam);

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);

		pCubeMap->Render();
	}

	g_pUDA->EndEvent();
}
