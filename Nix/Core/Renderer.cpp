#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "RenderStates.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
#include "NXPassShadowMap.h"
#include "NXCubeMap.h"

void Renderer::Init()
{
	NXGlobalInputLayout::Init();
	NXGlobalBufferManager::Init();

	InitRenderer();

	m_scene = new NXScene();
	m_scene->Init();

	// forward or deferred renderer?
	{
		m_pForwardRenderer = new NXForwardRenderer(m_scene);
		m_pForwardRenderer->Init();

		m_pDeferredRenderer = new NXDeferredRenderer(m_scene);
		m_pDeferredRenderer->Init();

		// 这个bool将来做成Settings（配置文件）之类的结构。
		m_isDeferredShading = false;
	}

	m_pPassShadowMap = new NXPassShadowMap(m_scene);
	m_pPassShadowMap->Init(2048, 2048);
}

void Renderer::InitRenderer()
{
	// create VS & IL
	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\RenderTarget.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderOffScreen));

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
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\RenderTarget.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderOffScreen));

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
	// 更新场景Scripts。实际上是用Scripts控制指定物体的Transform。
	m_scene->UpdateScripts();

	// 更新Transform
	m_scene->UpdateTransform();

	// 更新Camera的常量缓存数据（VP矩阵、眼睛位置）
	m_scene->UpdateCamera();
}

void Renderer::DrawScene()
{
	g_pUDA->BeginEvent(L"Render Scene");

	// 渲染主场景所用的Sampler
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

	// 设置两个RTV，一个用于绘制主场景，一个用于绘制显示区
	auto pOffScreenRTV = g_dxResources->GetRTVOffScreen();
	auto pRenderTargetView = g_dxResources->GetRenderTargetView();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();

	// 设置视口
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));
	
	// 切换到主RTV，并清空主RTV和DSV
	g_pContext->OMSetRenderTargets(1, &pOffScreenRTV, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pOffScreenRTV, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	DrawPrimitives();

	// 绘制CubeMap
	g_pContext->RSSetState(RenderStates::NoCullRS.Get());
	g_pContext->OMSetDepthStencilState(RenderStates::CubeMapDSS.Get(), 0);
	DrawCubeMap();

	// 以上操作全部都是在主RTV中进行的。
	// 下面切换到QuadRTV，简单来说就是将主RTV绘制到这个RTV，然后作为一张四边形纹理进行最终输出。
	g_pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pRenderTargetView, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pUDA->BeginEvent(L"Render Target");
	{
		g_pContext->RSSetState(nullptr);
		g_pContext->OMSetDepthStencilState(nullptr, 0);

		g_pContext->IASetInputLayout(m_pInputLayoutPNT.Get());
		g_pContext->VSSetShader(m_pVertexShaderOffScreen.Get(), nullptr, 0);
		g_pContext->PSSetShader(m_pPixelShaderOffScreen.Get(), nullptr, 0);

		// 绘制主渲染屏幕RTV：
		m_renderTarget->Render();
	}
	g_pUDA->EndEvent();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[2] = { nullptr };
	g_pContext->PSSetShaderResources(0, 2, pNullSRV);

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(1, 0, &parameters));

	g_pUDA->EndEvent();
}

void Renderer::Release()
{
	SafeRelease(m_pPassShadowMap);

	SafeDelete(m_pDeferredRenderer);
	SafeDelete(m_pForwardRenderer);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawPrimitives()
{
	if (!m_isDeferredShading)
	{
		m_pForwardRenderer->Render();
	}
	else
	{
		// Deferred shading
		m_pDeferredRenderer->RenderGBuffer();
		m_pDeferredRenderer->Render();
	}
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