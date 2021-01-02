#include "NXGBuffer.h"
#include "GlobalBufferManager.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXScene.h"

NXGBuffer::NXGBuffer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBuffer::~NXGBuffer()
{
}

void NXGBuffer::Init()
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

	// 创建Tex+SRV+RTV
	// 现行G-Buffer结构如下：
	// RT0:		Position	R11G11B10
	// RT1:		Normal		R11G11B10
	// RT2:		Albedo		R11G11B10
	// RT3:		Specular	R11G11B10
	Vector2 sz = g_dxResources->GetViewSize();

	// 创建Tex
	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R11G11B10_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTex[0]);
	descTex.Format = DXGI_FORMAT_R11G11B10_FLOAT;
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTex[1]);
	descTex.Format = DXGI_FORMAT_R11G11B10_FLOAT;
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTex[2]);
	descTex.Format = DXGI_FORMAT_R11G11B10_FLOAT;
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTex[3]);

	// 创建SRV
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTex[0].Get(), &descSRV, &m_pSRV[0]);
	g_pDevice->CreateShaderResourceView(m_pTex[1].Get(), &descSRV, &m_pSRV[1]);
	g_pDevice->CreateShaderResourceView(m_pTex[2].Get(), &descSRV, &m_pSRV[2]);
	g_pDevice->CreateShaderResourceView(m_pTex[3].Get(), &descSRV, &m_pSRV[3]);

	// 创建RTV
	CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2D, descTex.Format);
	g_pDevice->CreateRenderTargetView(m_pTex[0].Get(), &descRTV, &m_pRTV[0]);
	g_pDevice->CreateRenderTargetView(m_pTex[1].Get(), &descRTV, &m_pRTV[1]);
	g_pDevice->CreateRenderTargetView(m_pTex[2].Get(), &descRTV, &m_pRTV[2]);
	g_pDevice->CreateRenderTargetView(m_pTex[3].Get(), &descRTV, &m_pRTV[3]);

	// 创建DSV
	ComPtr<ID3D11Texture2D> pTexDepthStencil;
	CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(D3D11_DSV_DIMENSION_TEXTURE2D);
	CD3D11_TEXTURE2D_DESC descTexDepth(DXGI_FORMAT_D32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL);
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&descTexDepth, nullptr, &pTexDepthStencil));
	g_pDevice->CreateDepthStencilView(pTexDepthStencil.Get(), &descDSV, &m_pDSV[0]); 
	g_pDevice->CreateDepthStencilView(pTexDepthStencil.Get(), &descDSV, &m_pDSV[1]);
	g_pDevice->CreateDepthStencilView(pTexDepthStencil.Get(), &descDSV, &m_pDSV[2]);
	g_pDevice->CreateDepthStencilView(pTexDepthStencil.Get(), &descDSV, &m_pDSV[3]);

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

	// RT0-RT3均使用LayoutPNTT顶点布局。
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutGBuffer));

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
}

void NXGBuffer::Generate()
{
}

void NXGBuffer::Render()
{
	g_pUDA->BeginEvent(L"GBuffer"); 

	Vector2 sz = g_dxResources->GetViewSize();
	g_pContext->RSSetViewports(1, &g_dxResources->GetViewPort());
	g_pContext->IASetInputLayout(m_pInputLayoutGBuffer.Get());

	g_pContext->ClearRenderTargetView(m_pRTV[0].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSV[0].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->OMSetRenderTargets(1, m_pRTV[0].GetAddressOf(), m_pDSV[0].Get());
	RenderRT0();

	g_pContext->ClearRenderTargetView(m_pRTV[1].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSV[1].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->OMSetRenderTargets(1, m_pRTV[1].GetAddressOf(), m_pDSV[1].Get());
	RenderRT1();

	g_pContext->ClearRenderTargetView(m_pRTV[2].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSV[2].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->OMSetRenderTargets(1, m_pRTV[2].GetAddressOf(), m_pDSV[2].Get());
	RenderRT2();

	g_pContext->ClearRenderTargetView(m_pRTV[3].Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSV[3].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->OMSetRenderTargets(1, m_pRTV[3].GetAddressOf(), m_pDSV[3].Get());
	RenderRT3();

	g_pUDA->EndEvent();
}

void NXGBuffer::RenderRT0()
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

void NXGBuffer::RenderRT1()
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

void NXGBuffer::RenderRT2()
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

void NXGBuffer::RenderRT3()
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

void NXGBuffer::Release()
{
}

void NXGBuffer::InitVertexIndexBuffer()
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
