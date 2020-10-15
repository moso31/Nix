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
	g_dxResources = std::make_shared<DirectResources>();
	g_dxResources->InitDevice();

	RenderStates::Init();

	m_pRenderer = std::make_shared<Renderer>();
	m_pRenderer->Init();
}

void App::Update()
{
	// ���³�������
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	// ������Ӱ��ͼ
	// m_pRenderer->DrawShadowMap();

	// ����������
	m_pRenderer->DrawScene();
}

void App::Release()
{
	if (m_pRenderer)
	{
		m_pRenderer->Release();
		m_pRenderer.reset();
	}

	RenderStates::Release();

	if (g_dxResources)
	{
		g_dxResources->ClearDevices();
		g_dxResources.reset();
	}

	NXEventKeyDown::GetInstance()->Release();
	NXEventKeyUp::GetInstance()->Release();
	NXEventMouseDown::GetInstance()->Release();
	NXEventMouseUp::GetInstance()->Release();
	NXEventMouseMove::GetInstance()->Release();
	NXEventKeyDown::GetInstance().reset();
	NXEventKeyUp::GetInstance().reset();
	NXEventMouseDown::GetInstance().reset();
	NXEventMouseUp::GetInstance().reset();
	NXEventMouseMove::GetInstance().reset();
}