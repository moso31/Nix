#include "Scene.h"
#include "Box.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "Camera.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::Init()
{
	auto pPrimitive = make_shared<Cylinder>();
	pPrimitive->Init(2.0f, 2.0f, 44, 7);
	m_primitives.push_back(pPrimitive);

	auto pCamera = make_shared<Camera>();
	pCamera->Init();
	m_mainCamera = pCamera;
}

void Scene::Update()
{
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		auto pPrim = *it;
		pPrim->Update();
	}
	m_mainCamera->Update();
}

void Scene::Render()
{
	m_mainCamera->Render();

	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		auto pPrim = *it;
		pPrim->Render();
	}
}

void Scene::Release()
{
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		(*it)->Release();
		it->reset();
	}

	m_mainCamera->Release();
	m_mainCamera.reset();
}
