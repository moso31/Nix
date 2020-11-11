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

	m_pPassShadowMap = new NXPassShadowMap(m_scene);
	m_pPassShadowMap->Init(2048, 2048);
}

void Renderer::InitRenderer()
{
	// create VS & IL
	ID3DBlob* pVSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "VS", "vs_5_0", &pVSBlob), 
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutPNTT));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\RenderTarget.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderOffScreen));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderShadowMap));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutPNT));
	pVSBlob->Release();

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderCubeMap)); 

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutP));
	pVSBlob->Release();

	// Create PS
	ID3DBlob* pPSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));

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
	pPSBlob->Release();

	// Create Sampler
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinearWrap));

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinearClamp));

	// shadow map ר�� PCF �˲���������
	D3D11_SAMPLER_DESC sampShadowMapPCFDesc;
	ZeroMemory(&sampShadowMapPCFDesc, sizeof(sampShadowMapPCFDesc));
	sampShadowMapPCFDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampShadowMapPCFDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;	// ���ñ߽�Ѱַ
	sampShadowMapPCFDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampShadowMapPCFDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	for (int i = 0; i < 4; i++) 
		sampShadowMapPCFDesc.BorderColor[i] = 0.0f;	// �����߽粿��Ϊ��ɫ
	sampShadowMapPCFDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampShadowMapPCFDesc, &m_pSamplerShadowMapPCF));

	// Create RenderTarget
	m_renderTarget = new NXRenderTarget();
	m_renderTarget->Init();

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pContext->RSSetState(nullptr);	// back culling
	g_pContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	g_pContext->PSSetSamplers(0, 1, &m_pSamplerLinearWrap);
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
}

void Renderer::DrawScene()
{
	// ʹ��Ĭ�ϱ����޳���ָ����Ϊ�վ���Ĭ�ϵ�back culling��
	g_pContext->RSSetState(nullptr);

	// ��Ⱦ���������õ�Sampler
	g_pContext->PSSetSamplers(0, 1, &m_pSamplerLinearWrap);

	// ��������RTV��һ�����ڻ�����������һ�����ڻ�����ʾ��
	auto pOffScreenRTV = g_dxResources->GetOffScreenRTV();
	auto pRenderTargetView = g_dxResources->GetRenderTargetView();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();

	// �����ӿ�
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));
	
	// �л�����RTV���������RTV��DSV
	g_pContext->OMSetRenderTargets(1, &pOffScreenRTV, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pOffScreenRTV, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// ����Camera�ĳ����������ݣ�VP�����۾�λ�ã�
	m_scene->UpdateCamera();

	// ����CubeMap
	g_pContext->IASetInputLayout(m_pInputLayoutP);
	g_pContext->RSSetState(RenderStates::NoCullRS);
	g_pContext->OMSetDepthStencilState(RenderStates::CubeMapDSS, 0);
	DrawCubeMap();

	// ����Primitives
	g_pContext->IASetInputLayout(m_pInputLayoutPNTT);
	g_pContext->RSSetState(nullptr);
	g_pContext->OMSetDepthStencilState(nullptr, 0);
	DrawPrimitives();

	// ���ϲ���ȫ����������RTV�н��еġ�
	// �����л���QuadRTV������˵���ǽ���RTV���Ƶ����RTV��Ȼ����Ϊһ���ı�������������������
	g_pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pRenderTargetView, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->IASetInputLayout(m_pInputLayoutPNT);
	g_pContext->VSSetShader(m_pVertexShaderOffScreen, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderOffScreen, nullptr, 0);

	// ��������Ⱦ��ĻRTV��
	m_renderTarget->Render();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[2] = { nullptr };
	g_pContext->PSSetShaderResources(0, 2, pNullSRV);

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(1, 0, &parameters));
}

void Renderer::Release()
{
	SafeRelease(m_pPassShadowMap);

	SafeReleaseCOM(m_pInputLayoutP);
	SafeReleaseCOM(m_pInputLayoutPNT);
	SafeReleaseCOM(m_pInputLayoutPNTT);
	SafeReleaseCOM(m_pVertexShader);
	SafeReleaseCOM(m_pPixelShader);
	SafeReleaseCOM(m_pSamplerLinearWrap);
	SafeReleaseCOM(m_pSamplerLinearClamp);
	SafeReleaseCOM(m_pSamplerShadowMapPCF);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawPrimitives()
{
	// ����ʹ�õ�VS��PS��������scene.fx��
	g_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	// ������ȾBuffer��Slot����ʵ�ܽ��һ�仰���ǣ�
	// ��VS/PS��CB/SRV/Sampler��xxx�Ųۣ�Slot����������xxx��
	g_pContext->VSSetConstantBuffers(1, 1, &NXGlobalBufferManager::m_cbCamera);
	g_pContext->PSSetConstantBuffers(1, 1, &NXGlobalBufferManager::m_cbCamera);

	auto pCbLights = m_scene->GetConstantBufferLights();
	g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	// PBR��ġ���Ӱ��ͼ��ʱͣ�á�
	//auto pShadowMapSRV = m_pPassShadowMap->GetSRV();
	//g_pContext->PSSetShaderResources(10, 1, &pShadowMapSRV);

	auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
	g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);

	for (auto pPrim : m_scene->GetPrimitives())
	{
		// ������Ⱦ����������Mesh��
		// ��������CB/SRV��Slot�����䣬��ÿ�������World��Material��Tex��Ϣ�����ܸı䡣
		// ���Խ�һ���Ż������簴Material���ơ���Tex���ơ�
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, &NXGlobalBufferManager::m_cbObject);

		auto pMat = pPrim->GetPBRMaterial();
		if (pMat)
		{
			auto pSRVAlbedo = pMat->GetSRVAlbedo();
			g_pContext->PSSetShaderResources(1, 1, &pSRVAlbedo);

			auto pSRVNormal = pMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(2, 1, &pSRVNormal);

			auto pSRVMetallic = pMat->GetSRVMetallic();
			g_pContext->PSSetShaderResources(3, 1, &pSRVMetallic);

			auto pSRVRoughness = pMat->GetSRVRoughness();
			g_pContext->PSSetShaderResources(4, 1, &pSRVRoughness);

			auto pSRVAO = pMat->GetSRVAO();
			g_pContext->PSSetShaderResources(5, 1, &pSRVAO);

			auto pCBMaterial = pPrim->GetMaterialBuffer();
			g_pContext->PSSetConstantBuffers(3, 1, &pCBMaterial);
		}
		pPrim->Render();
	}
}

void Renderer::DrawCubeMap()
{
	g_pContext->VSSetShader(m_pVertexShaderCubeMap, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderCubeMap, nullptr, 0);

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update();
		g_pContext->VSSetConstantBuffers(0, 1, &NXGlobalBufferManager::m_cbObject);

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		auto pIrradianceMapSRV = pCubeMap->GetSRVIrradianceMap();
		auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
		auto pBRDF2DLUT = pCubeMap->GetSRVBRDF2DLUT();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(8, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(9, 1, &pBRDF2DLUT);

		pCubeMap->Render();
	}
}

void Renderer::DrawShadowMap()
{
	g_pContext->VSSetShader(m_pVertexShaderShadowMap, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderShadowMap, nullptr, 0);
	g_pContext->PSSetSamplers(1, 1, &m_pSamplerShadowMapPCF);

	g_pContext->RSSetState(RenderStates::ShadowMapRS);
	m_pPassShadowMap->Load();
	m_pPassShadowMap->UpdateConstantBuffer();
	m_pPassShadowMap->Render();
}