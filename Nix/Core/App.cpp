#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"

#include "App.h"
#include "Renderer.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXEvent.h"

App::App() :
	m_pRenderer(nullptr),
	m_lastViewSize(0.0f, 0.0f),
	m_viewSize(20.0f, 12.0f)
{

}

void App::Init()
{
	m_pDXResources = new DirectResources();
	m_pDXResources->InitDevice();

	m_pRenderer = new Renderer(m_viewSize);
	m_pRenderer->Init();
}

void App::OnWindowResize(UINT width, UINT height)
{
	if (width & height)
	{
		m_pDXResources->OnResize(width, height);
		//m_pRenderer->OnResize(Vector2((float)width, (float)height));
	}
}

void App::OnResize(const Vector2& rtSize)
{
	if ((UINT)rtSize.x & (UINT)rtSize.y)
	{
		m_pRenderer->OnResize(rtSize);
	}
}

void App::FrameBegin()
{
	m_pDXResources->FrameBegin();
}

void App::ResizeCheck()
{
	if (fabsf(m_lastViewSize.x - m_viewSize.x) > 0.01f || fabsf(m_lastViewSize.y - m_viewSize.y) > 0.01f)
	{
		OnResize(m_viewSize);
		m_lastViewSize = m_viewSize;
	}
}

void App::Reload()
{
	m_pRenderer->ResourcesReloading();
}

void App::Update()	
{
	m_pRenderer->Update();
}

void App::Draw()
{
	// »­äÖÈ¾Ö¡
	m_pRenderer->RenderFrame();

	// »­UI
	m_pRenderer->RenderGUI(m_pDXResources->GetCurrentSwapChain());

	m_pRenderer->ClearAllPSResources();
}

void App::FrameEnd()
{
	m_pDXResources->FrameEnd();
}

void App::Release()
{
	SafeRelease(m_pRenderer);
	NXResourceManager::GetInstance()->Release();
	NXShaderComplier::GetInstance()->Release();
	SafeRelease(m_pDXResources);
}