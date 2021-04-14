#include "NXDepthPrepass.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXScene.h"

NXDepthPrepass::NXDepthPrepass(NXScene* pScene) :
	m_pInputLayout(nullptr),
	m_pVertexShader(nullptr),
	m_pPixelShader(nullptr),
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
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));

	ComPtr<ID3D11Texture2D> pTexNormal;
	CD3D11_TEXTURE2D_DESC desc(
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		lround(DepthBufferSize.x), 
		lround(DepthBufferSize.y), 
		1, 
		1, 
		D3D11_BIND_SHADER_RESOURCE
	);
	g_pDevice->CreateTexture2D(&desc, nullptr, &pTexNormal);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexNormal.Get(), nullptr, &m_pSRVNormal));
}

void NXDepthPrepass::Render(ID3D11DepthStencilView* pDSVDepth)
{
	g_pUDA->BeginEvent(L"Depth Prepass");

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();	// 绘制主场景的RTV
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepth);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		auto pMat = pPrim->GetPBRMaterial();
		if (pMat)
		{
			auto pSRVNormal = pMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(0, 1, &pSRVNormal);
		}

		// TODO：实际上 DepthPrepass 不用传入所有材质，只需要传入normal就行了。回头改改这块。
		auto pCBMaterial = pPrim->GetMaterialBuffer();
		g_pContext->PSSetConstantBuffers(2, 1, &pCBMaterial);

		pPrim->Render();
	}

	g_pUDA->EndEvent();
}
