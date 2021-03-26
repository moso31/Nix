#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"

#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::Init()
{
	float scale = 1.0f;
	// Create vertex buffer
	m_vertices =
	{
		// -Z
		{ Vector3(-scale, +scale, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+scale, +scale, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(+scale, -scale, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-scale, -scale, 0.0f), Vector2(0.0f, 1.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitVertexIndexBuffer();

	// 现行G-Buffer结构如下：
	// RT0:		Position				R32G32B32A32_FLOAT
	// RT1:		Normal					R32G32B32A32_FLOAT
	// RT2:		Albedo					R10G10B10A2_UNORM
	// RT3:		Metallic+Roughness+AO	R10G10B10A2_UNORM
	// *注意：上述RT0、RT1现在用的是128位浮点数——这只是临时方案。RT2、RT3也有待商榷。

	Vector2 sz = g_dxResources->GetViewSize();

	// 创建Tex
	CD3D11_TEXTURE2D_DESC descTex0(DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	CD3D11_TEXTURE2D_DESC descTex1(DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	CD3D11_TEXTURE2D_DESC descTex2(DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	CD3D11_TEXTURE2D_DESC descTex3(DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	g_pDevice->CreateTexture2D(&descTex0, nullptr, &m_pTex[0]);
	g_pDevice->CreateTexture2D(&descTex1, nullptr, &m_pTex[1]);
	g_pDevice->CreateTexture2D(&descTex2, nullptr, &m_pTex[2]);
	g_pDevice->CreateTexture2D(&descTex3, nullptr, &m_pTex[3]);

	// 创建SRV
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV0(D3D11_SRV_DIMENSION_TEXTURE2D, descTex0.Format, 0, descTex0.MipLevels, 0, descTex0.ArraySize);
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV1(D3D11_SRV_DIMENSION_TEXTURE2D, descTex1.Format, 0, descTex1.MipLevels, 0, descTex1.ArraySize);
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV2(D3D11_SRV_DIMENSION_TEXTURE2D, descTex2.Format, 0, descTex2.MipLevels, 0, descTex2.ArraySize);
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV3(D3D11_SRV_DIMENSION_TEXTURE2D, descTex3.Format, 0, descTex3.MipLevels, 0, descTex3.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTex[0].Get(), &descSRV0, &m_pSRV[0]);
	g_pDevice->CreateShaderResourceView(m_pTex[1].Get(), &descSRV1, &m_pSRV[1]);
	g_pDevice->CreateShaderResourceView(m_pTex[2].Get(), &descSRV2, &m_pSRV[2]);
	g_pDevice->CreateShaderResourceView(m_pTex[3].Get(), &descSRV3, &m_pSRV[3]);

	// 创建RTV
	CD3D11_RENDER_TARGET_VIEW_DESC descRTV0(D3D11_RTV_DIMENSION_TEXTURE2D, descTex0.Format);
	CD3D11_RENDER_TARGET_VIEW_DESC descRTV1(D3D11_RTV_DIMENSION_TEXTURE2D, descTex1.Format);
	CD3D11_RENDER_TARGET_VIEW_DESC descRTV2(D3D11_RTV_DIMENSION_TEXTURE2D, descTex2.Format);
	CD3D11_RENDER_TARGET_VIEW_DESC descRTV3(D3D11_RTV_DIMENSION_TEXTURE2D, descTex3.Format);
	g_pDevice->CreateRenderTargetView(m_pTex[0].Get(), &descRTV0, &m_pRTV[0]);
	g_pDevice->CreateRenderTargetView(m_pTex[1].Get(), &descRTV1, &m_pRTV[1]);
	g_pDevice->CreateRenderTargetView(m_pTex[2].Get(), &descRTV2, &m_pRTV[2]);
	g_pDevice->CreateRenderTargetView(m_pTex[3].Get(), &descRTV3, &m_pRTV[3]);

	// 创建DSV
	CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(D3D11_DSV_DIMENSION_TEXTURE2D);
	CD3D11_TEXTURE2D_DESC descTexDepth(DXGI_FORMAT_D24_UNORM_S8_UINT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL);
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&descTexDepth, nullptr, &m_pTexDepth));
	g_pDevice->CreateDepthStencilView(m_pTexDepth.Get(), &descDSV, &m_pDSVDepth);

	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader[0]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader[1]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader[2]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader[3]));

	// 对RT0-RT3使用LayoutPNTT顶点布局。
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutGBuffer));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DeferredRender.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderRender));
	
	// 对最终渲染使用LayoutPT顶点布局。
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutRender));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "PS_RT0", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[0]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "PS_RT1", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[1]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "PS_RT2", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[2]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "PS_RT3", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[3]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DeferredRender.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderRender));
}

void NXDeferredRenderer::RenderGBuffer()
{
	g_pContext->IASetInputLayout(m_pInputLayoutGBuffer.Get());

	g_pUDA->BeginEvent(L"GBuffer");
	g_pContext->OMSetRenderTargets(1, m_pRTV[0].GetAddressOf(), m_pDSVDepth.Get());
	g_pContext->ClearRenderTargetView(m_pRTV[0].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	RenderRT0();

	g_pContext->OMSetRenderTargets(1, m_pRTV[1].GetAddressOf(), m_pDSVDepth.Get());
	g_pContext->ClearRenderTargetView(m_pRTV[1].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	RenderRT1();

	g_pContext->OMSetRenderTargets(1, m_pRTV[2].GetAddressOf(), m_pDSVDepth.Get());
	g_pContext->ClearRenderTargetView(m_pRTV[2].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	RenderRT2();

	g_pContext->OMSetRenderTargets(1, m_pRTV[3].GetAddressOf(), m_pDSVDepth.Get());
	g_pContext->ClearRenderTargetView(m_pRTV[3].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	RenderRT3();

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::RenderRT0()
{
	g_pUDA->BeginEvent(L"Render RT0");

	// 设置使用的VS和PS（scene.fx）
	g_pContext->VSSetShader(m_pVertexShader[0].Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader[0].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives()) 
	{
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

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

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::RenderRT1()
{
	g_pUDA->BeginEvent(L"Render RT1");

	// 设置使用的VS和PS（scene.fx）
	g_pContext->VSSetShader(m_pVertexShader[1].Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader[1].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

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

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::RenderRT2()
{
	g_pUDA->BeginEvent(L"Render RT2");

	// 设置使用的VS和PS（scene.fx）
	g_pContext->VSSetShader(m_pVertexShader[2].Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader[2].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

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

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::RenderRT3()
{
	g_pUDA->BeginEvent(L"Render RT3");

	// 设置使用的VS和PS（scene.fx）
	g_pContext->VSSetShader(m_pVertexShader[3].Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader[3].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

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

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Render()
{
	g_pUDA->BeginEvent(L"Deferred rendering");
	g_pContext->IASetInputLayout(m_pInputLayoutRender.Get());
	g_pContext->VSSetShader(m_pVertexShaderRender.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderRender.Get(), nullptr, 0);

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	auto pCbLights = m_pScene->GetConstantBufferLights();
	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCbLights)
		g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	if (pCubeMap)
	{
		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		auto pIrradianceMapSRV = pCubeMap->GetSRVIrradianceMap();
		auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
		auto pBRDF2DLUT = pCubeMap->GetSRVBRDF2DLUT();
		g_pContext->PSSetShaderResources(4, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(5, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(6, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pBRDF2DLUT);
	}

	auto pOffScreenRTV = g_dxResources->GetRTVOffScreen();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();
	g_pContext->OMSetRenderTargets(1, &pOffScreenRTV, pDepthStencilView);

	g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
	g_pContext->PSSetShaderResources(0, 1, m_pSRV[0].GetAddressOf());
	g_pContext->PSSetShaderResources(1, 1, m_pSRV[1].GetAddressOf());
	g_pContext->PSSetShaderResources(2, 1, m_pSRV[2].GetAddressOf());
	g_pContext->PSSetShaderResources(3, 1, m_pSRV[3].GetAddressOf());

	UINT stride = sizeof(VertexPT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);

	auto pTexDepthStencil = g_dxResources->GetTexDepthStencil();
	g_pContext->CopyResource(pTexDepthStencil, m_pTexDepth.Get());

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
}
