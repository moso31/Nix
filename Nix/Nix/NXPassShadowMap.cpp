#include "NXPassShadowMap.h"
#include "GlobalBufferManager.h"

#include "NXScene.h"
#include "NXPrimitive.h"

NXPassShadowMap::NXPassShadowMap(const std::shared_ptr<NXScene>& pScene) :
	m_pScene(pScene)
{
}

NXPassShadowMap::~NXPassShadowMap()
{
}

void NXPassShadowMap::Init(UINT width, UINT height)
{
	m_viewPort.TopLeftX = 0.0f;
	m_viewPort.TopLeftY = 0.0f;
	m_viewPort.Width = static_cast<float>(width);
	m_viewPort.Height = static_cast<float>(height);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	// 根据需要创建用于 3D 渲染的深度模具视图。
	CD3D11_TEXTURE2D_DESC descTex(
		DXGI_FORMAT_R24G8_TYPELESS,
		width,
		height,
		1, // 此深度模具视图只有一个纹理。
		1, // 使用单一 mipmap 级别。
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
	);

	ID3D11Texture2D* pTexShadowMap;
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&descTex, nullptr, &pTexShadowMap));

	CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(
		D3D11_DSV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_D24_UNORM_S8_UINT
	);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(pTexShadowMap, &descDSV, &m_pDepthDSV));

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(
		D3D11_SRV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		0,
		descTex.MipLevels,
		0,
		0
	);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexShadowMap, &descSRV, &m_pDepthSRV));

	if (pTexShadowMap) pTexShadowMap->Release();

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferShadowMapTransform);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbTransform));

	// 设置常量缓存
	m_cbDataTransform = NXGlobalBufferManager::m_cbDataShadowMap;
}

void NXPassShadowMap::Load()
{
	g_pContext->RSSetViewports(1, &m_viewPort);

	// 无需RenderTarget，只绘制DSV
	ID3D11RenderTargetView* renderTargets[1] = { nullptr };
	g_pContext->OMSetRenderTargets(1, renderTargets, m_pDepthDSV);
	g_pContext->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void NXPassShadowMap::UpdateConstantBuffer()
{
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject, 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);
	g_pContext->UpdateSubresource(m_cbTransform, 0, nullptr, &m_cbDataTransform, 0, 0);
}

void NXPassShadowMap::Render()
{
	g_pContext->VSSetConstantBuffers(1, 1, &m_cbTransform);
	g_pContext->PSSetConstantBuffers(1, 1, &m_cbTransform);

	auto pPrims = m_pScene->GetPrimitives();
	for (auto it = pPrims.begin(); it != pPrims.end(); it++)
	{
		auto p = *it;
		p->Update();
		auto pTexSRV = p->GetTextureSRV();
		auto pMaterial = p->GetMaterialBuffer();
		g_pContext->VSSetConstantBuffers(0, 1, &NXGlobalBufferManager::m_cbObject);
		g_pContext->PSSetShaderResources(0, 1, &pTexSRV);
		g_pContext->PSSetConstantBuffers(3, 1, &pMaterial);
		p->Render();
	}
}

void NXPassShadowMap::Release()
{
	if (m_pDepthDSV)
		m_pDepthDSV->Release();

	if (m_pDepthSRV)
		m_pDepthSRV->Release();

	if (m_cbTransform)
		m_cbTransform->Release();
}
