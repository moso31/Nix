#include "App.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXEvent.h"

App::App() :
	m_pRenderer(nullptr)
{

}

void App::Init()
{
	m_pDXResources = new DirectResources();
	m_pDXResources->InitDevice();

	m_pRenderer = new Renderer(m_pDXResources);
	m_pRenderer->Init();

	m_pRenderer->InitGUI();
}

void App::OnResize(UINT width, UINT height)
{
	if (width & height)
	{
		m_pDXResources->OnResize(width, height);
		m_pRenderer->OnResize(Vector2((float)width, (float)height));
	}
}

void App::Reload()
{
	m_pRenderer->ResourcesReloading();
}

void App::Update()
{
	// 更新场景数据
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->OMSetDepthStencilState(nullptr, 0);
	g_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	g_pContext->RSSetState(nullptr);	// back culling

	m_pRenderer->RenderFrame();

	m_pDXResources->PrepareToRenderGUI();
	m_pRenderer->RenderGUI();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
	g_pContext->PSSetShaderResources(0, 16, pNullSRV);

	// Present() 向用户呈现渲染图像。
	// 在Present之前，将所有GPU队列中的内容全部执行完，否则可能会出现渲染问题。
	// （比如切换CubeMapSRV-Preview2D纹理后未能在Present之前及时加载导致程序崩溃）。
	ComPtr<ID3D11Query> pQuery;
	g_pDevice->CreateQuery(&CD3D11_QUERY_DESC(D3D11_QUERY_EVENT, 0), &pQuery);
	g_pContext->End(pQuery.Get());

	while (g_pContext->GetData(pQuery.Get(), nullptr, 0, 0) != S_OK)
	{
	}

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(0, 0, &parameters));
}

void App::ReleaseUnusedTextures()
{
	NXResourceManager::GetInstance()->GetTextureManager()->ReleaseUnusedTextures();
}

void App::Release()
{
	SafeRelease(m_pRenderer);
	NXResourceManager::GetInstance()->Release();
	NXShaderComplier::GetInstance()->Release();
	SafeRelease(m_pDXResources);
}