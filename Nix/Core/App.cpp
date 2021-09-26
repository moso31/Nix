#include "App.h"
#include "DirectResources.h"
#include "RenderStates.h"
#include "NXEvent.h"

App::App() :
	m_pRenderer(nullptr)
{

}

void App::Init()
{
	g_dxResources = new DirectResources();
	g_dxResources->InitDevice();

	RenderStates::Init();

	m_pRenderer = new Renderer();
	m_pRenderer->Init();

	m_pRenderer->InitGUI();
}

void App::Update()
{
	// 更新场景数据
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	// 绘制主场景
	m_pRenderer->DrawScene();
	m_pRenderer->DrawGUI();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
	g_pContext->PSSetShaderResources(0, 16, pNullSRV);

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(1, 0, &parameters));
}

void App::Release()
{
	SafeRelease(m_pRenderer);

	RenderStates::Release();

	SafeRelease(g_dxResources);
}