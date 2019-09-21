#include "NXShadowMap.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXShadowMap::NXShadowMap() :
	m_pDepthDSV(nullptr),
	m_pDepthSRV(nullptr)
{
}

NXShadowMap::~NXShadowMap()
{
}

void NXShadowMap::UpdateConstantBuffer(const Matrix& viewMatrix, const Matrix& projMatrix, const Matrix& texMatrix)
{
	m_cbShadowMapCameraData.view = viewMatrix.Transpose();
	m_cbShadowMapCameraData.projection = projMatrix.Transpose();
	m_cbShadowMapCameraData.texture = texMatrix.Transpose();
	g_pContext->UpdateSubresource(m_cbShadowMapCamera, 0, nullptr, &m_cbShadowMapCameraData, 0, 0);
	g_pContext->VSSetConstantBuffers(1, 1, &m_cbShadowMapCamera);
	g_pContext->PSSetConstantBuffers(1, 1, &m_cbShadowMapCamera);
}

void NXShadowMap::Init(UINT width, UINT height)
{
	m_viewPort.TopLeftX = 0.0f;
	m_viewPort.TopLeftY = 0.0f;
	m_viewPort.Width = static_cast<float>(width);
	m_viewPort.Height = static_cast<float>(height);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	// 根据需要创建用于 3D 渲染的深度模具视图。
	CD3D11_TEXTURE2D_DESC1 descTex(
		DXGI_FORMAT_R24G8_TYPELESS,
		width,
		height,
		1, // 此深度模具视图只有一个纹理。
		1, // 使用单一 mipmap 级别。
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
	);

	ID3D11Texture2D1* pTexShadowMap;
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D1(&descTex, nullptr, &pTexShadowMap));

	CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(
		D3D11_DSV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_D24_UNORM_S8_UINT
	);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(pTexShadowMap, &descDSV, &m_pDepthDSV));

	CD3D11_SHADER_RESOURCE_VIEW_DESC1 descSRV(
		D3D11_SRV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		0,
		descTex.MipLevels,
		0,
		0
	);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView1(pTexShadowMap, &descSRV, &m_pDepthSRV));

	if (pTexShadowMap) pTexShadowMap->Release();

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferShadowMapCamera);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbShadowMapCamera));
}

void NXShadowMap::Update()
{
}

void NXShadowMap::Render(const shared_ptr<Scene>& pTargetScene)
{
	g_pContext->RSSetViewports(1, &m_viewPort); 

	// 无需RenderTarget，只绘制DSV
	ID3D11RenderTargetView* renderTargets[1] = { nullptr };
	g_pContext->OMSetRenderTargets(1, renderTargets, m_pDepthDSV);
	g_pContext->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void NXShadowMap::Release()
{
	if (m_pDepthDSV)
		m_pDepthDSV->Release();

	if (m_pDepthSRV)
		m_pDepthSRV->Release();
}
