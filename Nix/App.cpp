#include "App.h"
#include "DirectResources.h"

App::App() :
	m_pRenderer(nullptr)
{

}

void App::Init()
{
	g_dxResources = std::make_shared<DirectResources>();
	if (FAILED(g_dxResources->InitDevice()))
	{
		g_dxResources->ClearDevices();
		return;
	}

	m_pRenderer = make_shared<Renderer>();
	m_pRenderer->InitRenderer();
}

void App::Update()
{
	m_pRenderer->Update();
}

void App::Render()
{
	m_pRenderer->Render();
}

void App::Release()
{
	g_dxResources->ClearDevices();
}