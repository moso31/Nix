#include "NXDepthPrepass.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXDepthPrepass::NXDepthPrepass(NXScene* pScene) :
	m_pInputLayout(nullptr),
	m_pScene(pScene)
{
}

NXDepthPrepass::~NXDepthPrepass()
{
}

void NXDepthPrepass::Init(const Vector2& DepthBufferSize)
{
	// create VS & IL
	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "PS0", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[0]));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "PS1", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader[1]));

	ComPtr<ID3D11Texture2D> pTexPosition;
	ComPtr<ID3D11Texture2D> pTexNormal;
	CD3D11_TEXTURE2D_DESC desc(
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		lround(DepthBufferSize.x),
		lround(DepthBufferSize.y),
		1,
		1,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
	);
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&desc, nullptr, &pTexNormal));
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexNormal.Get(), nullptr, &m_pSRVNormal));
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(pTexNormal.Get(), nullptr, &m_pRTVNormal));
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&desc, nullptr, &pTexPosition));
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexPosition.Get(), nullptr, &m_pSRVPosition));
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(pTexPosition.Get(), nullptr, &m_pRTVPosition));

	CD3D11_TEXTURE2D_DESC descDepthPrepass(
		DXGI_FORMAT_R24G8_TYPELESS, // 无法在Tex直接确定使用哪种format，因为SRV和DSV的格式不同。
		lround(DepthBufferSize.x),
		lround(DepthBufferSize.y),
		1,
		1,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL
	);
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&descDepthPrepass, nullptr, &m_pTexDepthPrepass));
	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRVDepthPrepass(m_pTexDepthPrepass.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	CD3D11_DEPTH_STENCIL_VIEW_DESC descDSVDepthPrepass(m_pTexDepthPrepass.Get(), D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexDepthPrepass.Get(), &descSRVDepthPrepass, &m_pSRVDepthPrepass));
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexDepthPrepass.Get(), &descDSVDepthPrepass, &m_pDSVDepthPrepass));
}

void NXDepthPrepass::Render()
{
	g_pUDA->BeginEvent(L"Depth Prepass");

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	g_pContext->OMSetRenderTargets(1, m_pRTVNormal.GetAddressOf(), m_pDSVDepthPrepass.Get());
	g_pContext->ClearRenderTargetView(m_pRTVNormal.Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepthPrepass.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader[0].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		for (UINT i = 0; i < pPrim->GetSubMeshCount(); i++)
		{
			auto pSubMesh = pPrim->GetSubMesh(i);
			pSubMesh->Update();

			auto pMat = pSubMesh->GetPBRMaterial();
			auto pSRVNormal = pMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(0, 1, &pSRVNormal);

			auto pCBMaterial = pMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(2, 1, &pCBMaterial);

			pSubMesh->Render();
		}
	}

	g_pContext->OMSetRenderTargets(1, m_pRTVPosition.GetAddressOf(), m_pDSVDepthPrepass.Get());
	g_pContext->ClearRenderTargetView(m_pRTVPosition.Get(), Colors::Black);
	g_pContext->ClearDepthStencilView(m_pDSVDepthPrepass.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->PSSetShader(m_pPixelShader[1].Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		for (UINT i = 0; i < pPrim->GetSubMeshCount(); i++)
		{
			auto pSubMesh = pPrim->GetSubMesh(i);
			pSubMesh->Update();
			pSubMesh->Render();
		}
	}

	g_pUDA->EndEvent();
}
