#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "RenderStates.h"
#include "NXGUI.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
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
	m_scene->Init();

	m_pDepthPrepass = new NXDepthPrepass(m_scene);
	m_pDepthPrepass->Init(g_dxResources->GetViewSize());

	// forward or deferred renderer?
	{
		m_pForwardRenderer = new NXForwardRenderer(m_scene);
		m_pForwardRenderer->Init();

		m_pDeferredRenderer = new NXDeferredRenderer(m_scene);
		m_pDeferredRenderer->Init();

		// ���bool��������Settings�������ļ���֮��Ľṹ��
		m_isDeferredShading = true;
	}

	m_pPassShadowMap = new NXPassShadowMap(m_scene);
	m_pPassShadowMap->Init(2048, 2048);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	// create VS & IL
	ComPtr<ID3DBlob> pVSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderShadowMap));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutPNT));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderCubeMap)); 

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutP));

	// Create PS
	ComPtr<ID3DBlob> pPSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderShadowMap));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMap.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderCubeMap));

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

void Renderer::Preload()
{
	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->GenerateIrradianceMap();
		pCubeMap->GeneratePreFilterMap();
		pCubeMap->GenerateBRDF2DLUT(); 
	}
}

void Renderer::UpdateSceneData()
{
	// ���³���Scripts��ʵ��������Scripts����ָ�������Transform��
	m_scene->UpdateScripts();

	// ����Transform
	m_scene->UpdateTransform();

	// ����Camera�ĳ����������ݣ�VP�����۾�λ�ã�
	m_scene->UpdateCamera();
}

void Renderer::DrawScene()
{
	g_pUDA->BeginEvent(L"Render Scene");

	// ��Ⱦ���������õ�Sampler
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();	// ������������RTV
	auto pRTVFinalQuad = g_dxResources->GetRTVFinalQuad();	// ����������ȾQuad��RTV

	auto pSRVDepthStencil = g_dxResources->GetSRVDepthStencil();
	auto pDSVDepthStencil = g_dxResources->GetDSVDepthStencil();	

	// �����ӿ�
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));

	g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
	DrawDepthPrepass(pDSVDepthStencil);

	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
	g_pContext->ClearRenderTargetView(pRTVMainScene, Colors::WhiteSmoke);

	auto pSRVNormal = m_pDepthPrepass->GetSRVNormal();
	if (!m_isDeferredShading)
	{
		m_pSSAO->Render(pSRVNormal, pSRVDepthStencil);

		g_pContext->OMSetDepthStencilState(RenderStates::DSSForwardRendering.Get(), 0);
		m_pForwardRenderer->Render();
	}
	else
	{
		// Deferred shading
		g_pContext->OMSetDepthStencilState(nullptr, 0);
		m_pDeferredRenderer->RenderGBuffer();
		g_pContext->OMSetDepthStencilState(RenderStates::DSSDeferredRendering.Get(), 0);
		m_pDeferredRenderer->Render();

		m_pSSAO->Render(pSRVNormal, pSRVDepthStencil);
	}

	// ����CubeMap
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

void Renderer::DrawGUI()
{
	m_pGUI->Render();
}

void Renderer::Release()
{
	SafeRelease(m_pGUI);

	SafeDelete(m_pPassShadowMap);

	SafeDelete(m_pDeferredRenderer);
	SafeDelete(m_pForwardRenderer);

	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawDepthPrepass(ID3D11DepthStencilView* pDSVDepth)
{
	m_pDepthPrepass->Render(pDSVDepth);
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
		pCubeMap->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);

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