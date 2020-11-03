#include "App.h"
#include "DirectResources.h"
#include "RenderStates.h"
#include "NXEvent.h"
#include "NXRayTracer.h"

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
	m_pRenderer->Preload();
}

void App::Update()
{
	// ���³�������
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	// ����������
	m_pRenderer->DrawScene();
}

void App::Release()
{
	if (m_pRenderer)
	{
		m_pRenderer->Release();
		delete m_pRenderer;
	}

	RenderStates::Release();

	if (g_dxResources)
	{
		g_dxResources->ClearDevices();
		delete g_dxResources;
	}

	NXEventKeyDown::GetInstance()->Release();
	NXEventKeyUp::GetInstance()->Release();
	NXEventMouseDown::GetInstance()->Release();
	NXEventMouseUp::GetInstance()->Release();
	NXEventMouseMove::GetInstance()->Release();

	NXEventKeyDown::GetInstance()->Destroy();
	NXEventKeyUp::GetInstance()->Destroy();
	NXEventMouseDown::GetInstance()->Destroy();
	NXEventMouseUp::GetInstance()->Destroy();
	NXEventMouseMove::GetInstance()->Destroy();
}