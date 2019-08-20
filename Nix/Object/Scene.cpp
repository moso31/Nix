#include "Scene.h"
#include "Box.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "Camera.h"

// temp include.
#include "Light.h"
#include "Material.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::Init()
{
	auto pLight = make_shared<Light>();
	pLight->SetAmbient(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
	pLight->SetDiffuse(Vector4(0.5f, 0.5f, 0.5f, 1.0f));
	pLight->SetSpecular(Vector4(0.5f, 0.5f, 0.5f, 1.0f));	
	pLight->SetDirection(Vector3(1.0f, -1.0f, 1.0f));
	m_lights.push_back(pLight);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HRESULT hr = g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLight);
	if (FAILED(hr))
		return;
	m_cbDataLight.light = pLight->GetLightInfo();

	if (pLight)
	{
		ConstantBufferLight cb;
		cb.light = m_cbDataLight.light;
		g_pContext->UpdateSubresource(m_cbLight, 0, nullptr, &cb, 0, 0);
	}
	
	auto pMaterial = make_shared<Material>();
	pMaterial->SetAmbient(Vector4(0.7f, 0.85f, 0.7f, 1.0f));
	pMaterial->SetDiffuse(Vector4(0.7f, 0.85f, 0.7f, 1.0f));
	pMaterial->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 16.0f));
	m_materials.push_back(pMaterial);

	auto pPrimitive = make_shared<Cylinder>();
	pPrimitive->Init(2.0f, 2.0f, 44, 7);
	pPrimitive->SetMaterial(pMaterial);
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
