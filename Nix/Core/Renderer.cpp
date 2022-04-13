#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "RenderStates.h"
#include "NXGUI.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
#include "SceneManager.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXPassShadowMap.h"

void Renderer::Init()
{
	NXGlobalInputLayout::Init();
	NXGlobalBufferManager::Init();

	InitRenderer();

	m_scene = new NXScene();
	SceneManager::GetInstance()->SetWorkingScene(m_scene);
	m_scene->Init();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->GenerateIrradianceMap();
		pCubeMap->GeneratePreFilterMap();
		pCubeMap->GenerateBRDF2DLUT();
	}

	m_pDepthPrepass = new NXDepthPrepass(m_scene);
	m_pDepthPrepass->Init(g_dxResources->GetViewSize());

	m_pSSAO = new NXSimpleSSAO();
	m_pSSAO->Init(g_dxResources->GetViewSize());

	// forward or deferred renderer?
	{
		m_pForwardRenderer = new NXForwardRenderer(m_scene);
		m_pForwardRenderer->Init();

		m_pDeferredRenderer = new NXDeferredRenderer(m_scene);
		m_pDeferredRenderer->Init();

		// �����ġ����bool��������Settings��ini�����ļ���֮��ġ�
		m_isDeferredShading = true;
	}

	m_pPassShadowMap = new NXPassShadowMap(m_scene);
	m_pPassShadowMap->Init(2048, 2048);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, m_pSSAO);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	// �������ʼ��CommonRT��
	NXResourceManager::GetInstance()->InitCommonRT();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ShadowMap.fx", "VS", &m_pVertexShaderShadowMap, NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), &m_pInputLayoutPNT);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowMap.fx", "PS", &m_pPixelShaderShadowMap);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\CubeMap.fx", "VS", &m_pVertexShaderCubeMap, NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), &m_pInputLayoutP);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMap.fx", "PS", &m_pPixelShaderCubeMap);

	// Create RenderTarget
	m_renderTarget = new NXRenderTarget();
	m_renderTarget->Init();

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->RSSetState(nullptr);	// back culling
	g_pContext->OMSetDepthStencilState(nullptr, 0); 

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());
}

void Renderer::ResourcesReloading()
{
	NXResourceReloader::GetInstance()->Update();
}

void Renderer::UpdateSceneData()
{
	// ���³���Scripts��ʵ��������Scripts����ָ�������Transform��
	m_scene->UpdateScripts();

	// ����Transform
	m_scene->UpdateTransform();

	// ����Camera�ĳ����������ݣ�VP�����۾�λ�ã�
	m_scene->UpdateCamera();

	m_scene->UpdateLightData();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update();
	}

	m_pSSAO->Update();
}

void Renderer::RenderFrame()
{
	g_pUDA->BeginEvent(L"Render Scene");

	// ��Ⱦ���������õ�Sampler
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();	// ������������RTV
	auto pRTVFinalQuad = g_dxResources->GetRTVFinalQuad();	// ����������ȾQuad��RTV

	auto pSRVDepthStencil = g_dxResources->GetSRVDepthStencil();
	auto pDSVDepthStencil = g_dxResources->GetDSVDepthStencil();

	auto pSRVPosition = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer0)->GetSRV();
	auto pSRVNormal = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer1)->GetSRV();
	auto pDSVDepthPrepass = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	auto pSRVDepthPrepass = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ)->GetSRV();

	// �����ӿ�
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));

	// DepthPrepass
	//m_pDepthPrepass->Render();

	//if (!m_isDeferredShading)
	//{
	//	// SSAO
	//	m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	//	// Forward shading
	//	m_pForwardRenderer->Render(m_pSSAO->GetSRV());
	//}
	//else
	{
		// Deferred shading: RenderGBuffer
		m_pDeferredRenderer->RenderGBuffer();

		// SSAO
		m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

		// Deferred shading: Render
		m_pDeferredRenderer->Render(m_pSSAO->GetSRV());
	}

	// ����CubeMap
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
	g_pContext->OMSetDepthStencilState(RenderStates::DSSCubeMap.Get(), 0);
	DrawCubeMap();

	// ���ϲ���ȫ����������RTV�н��еġ�
	// �����л���QuadRTV������˵���ǽ���RTV���Ƶ����RTV��Ȼ����Ϊһ���ı�������������������
	g_pContext->OMSetRenderTargets(1, &pRTVFinalQuad, pDSVDepthStencil);
	g_pContext->ClearRenderTargetView(pRTVFinalQuad, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->RSSetState(nullptr);
	g_pContext->OMSetDepthStencilState(nullptr, 0);

	// ��������Ⱦ��ĻRTV��
	m_renderTarget->Render();

	g_pUDA->EndEvent();
}

void Renderer::RenderGUI()
{
	m_pGUI->Render();
}

void Renderer::Release()
{
	SafeRelease(m_pGUI);

	SafeDelete(m_pPassShadowMap);
	SafeRelease(m_pSSAO);

	SafeRelease(m_pDeferredRenderer);
	SafeDelete(m_pForwardRenderer);

	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::DrawCubeMap()
{
	g_pUDA->BeginEvent(L"Cube Map");
	g_pContext->IASetInputLayout(m_pInputLayoutP.Get());
	g_pContext->VSSetShader(m_pVertexShaderCubeMap.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderCubeMap.Get(), nullptr, 0);

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(1, 1, &pCBCubeMapParam);

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV); 

		auto pSRVIrradSH = pCubeMap->GetSRVIrradianceSH();
		g_pContext->PSSetShaderResources(1, 1, &pSRVIrradSH);

		pCubeMap->Render();
	}
	g_pUDA->EndEvent();
}

void Renderer::DrawShadowMap()
{
	g_pContext->VSSetShader(m_pVertexShaderShadowMap.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderShadowMap.Get(), nullptr, 0);
	g_pContext->PSSetSamplers(1, 1, RenderStates::SamplerShadowMapPCF.GetAddressOf());

	g_pContext->RSSetState(RenderStates::ShadowMapRS.Get());
	m_pPassShadowMap->Load();
	m_pPassShadowMap->UpdateConstantBuffer();
	m_pPassShadowMap->Render();
}