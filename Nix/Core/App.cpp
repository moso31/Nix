#include "App.h"
#include "DirectResources.h"
#include "RenderStates.h"

App::App() :
	m_pRenderer(nullptr)
{

}

void App::Init()
{
	g_dxResources = std::make_shared<DirectResources>();
	g_dxResources->InitDevice();

	RenderStates::Init();

	m_pRenderer = make_shared<Renderer>();
	m_pRenderer->Init();
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
	m_pRenderer->Release();
	g_dxResources->ClearDevices();
}