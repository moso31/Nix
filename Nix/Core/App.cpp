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
	// 更新场景数据
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	// 绘制主场景
	m_pRenderer->DrawScene();
}

void App::Release()
{
	SafeRelease(m_pRenderer);

	RenderStates::Release();

	SafeRelease(g_dxResources);

	NXEventKeyDown::GetInstance().Release();
	NXEventKeyUp::GetInstance().Release();
	NXEventMouseDown::GetInstance().Release();
	NXEventMouseUp::GetInstance().Release();
	NXEventMouseMove::GetInstance().Release();
}