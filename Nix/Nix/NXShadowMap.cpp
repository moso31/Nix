#include "NXShadowMap.h"


NXShadowMap::NXShadowMap()
{
}

NXShadowMap::~NXShadowMap()
{
}

void NXShadowMap::Init(UINT width, UINT height)
{
	// ������Ҫ�������� 3D ��Ⱦ�����ģ����ͼ��
	CD3D11_TEXTURE2D_DESC1 descTex(
		DXGI_FORMAT_R24G8_TYPELESS,
		width,
		height,
		1, // �����ģ����ͼֻ��һ������
		1, // ʹ�õ�һ mipmap ����
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
}

void NXShadowMap::Update()
{
}

void NXShadowMap::Render()
{
}
