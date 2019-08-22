#include "NXScene.h"
#include "NXBox.h"
#include "NXSphere.h"
#include "NXCylinder.h"
#include "NXPlane.h"
#include "NXCamera.h"

// temp include.
#include "NXLight.h"
#include "NXMaterial.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::Init()
{
	auto pDirLight = make_shared<NXDirectionalLight>();
	pDirLight->SetAmbient(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
	pDirLight->SetDiffuse(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pDirLight->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 1.0f));	
	pDirLight->SetDirection(Vector3(1.0f, -1.0f, 1.0f));
	m_lights.push_back(pDirLight);

	auto pPointLight = make_shared<NXPointLight>();
	pPointLight->SetAmbient(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
	pPointLight->SetDiffuse(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pPointLight->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pPointLight->SetPosition(Vector3(1.0f, 1.0f, -1.0f));
	pPointLight->SetRange(100.0f);
	pPointLight->SetAtt(Vector3(0.0f, 0.0f, 1.0f));
	m_lights.push_back(pPointLight);

	auto pSpotLight = make_shared<NXSpotLight>();
	pSpotLight->SetAmbient(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
	pSpotLight->SetDiffuse(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pSpotLight->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pSpotLight->SetPosition(Vector3(-1.0f, 1.0f, -1.0f));
	pSpotLight->SetRange(100.0f);
	pSpotLight->SetDirection(Vector3(1.0f, -1.0f, 1.0f));
	pSpotLight->SetSpot(1.0f);
	pSpotLight->SetAtt(Vector3(0.0f, 0.0f, 1.0f));
	m_lights.push_back(pSpotLight);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

	m_cbDataLights.dirLight = pDirLight->GetLightInfo();
	m_cbDataLights.pointLight = pPointLight->GetLightInfo();
	m_cbDataLights.spotLight = pSpotLight->GetLightInfo();

	if (pDirLight)
	{
		ConstantBufferLight cb;
		cb.dirLight = m_cbDataLights.dirLight;
		cb.pointLight = m_cbDataLights.pointLight;
		cb.spotLight = m_cbDataLights.spotLight;
		g_pContext->UpdateSubresource(m_cbLights, 0, nullptr, &cb, 0, 0);
	}
	
	auto pMaterial = make_shared<NXMaterial>();
	pMaterial->SetAmbient(Vector4(0.7f, 0.85f, 0.7f, 1.0f));
	pMaterial->SetDiffuse(Vector4(0.7f, 0.85f, 0.7f, 1.0f));
	pMaterial->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 16.0f));
	m_materials.push_back(pMaterial);

	auto pPrimitive = make_shared<NXPlane>();
	pPrimitive->Init(5.0f, 5.0f);
	pPrimitive->SetMaterial(pMaterial);
	m_primitives.push_back(pPrimitive);

	auto pCamera = make_shared<NXCamera>();
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
	g_pContext->PSSetConstantBuffers(2, 1, &m_cbLights);

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
