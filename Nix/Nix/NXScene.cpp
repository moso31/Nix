#include "NXScene.h"
#include "SceneManager.h"
#include "RenderStates.h"
//#include "HBVH.h"

#include "NXMesh.h"
#include "NXBox.h"
#include "NXSphere.h"
#include "NXCylinder.h"
#include "NXPlane.h"
#include "NXCamera.h"

//#include "NXShadowMap.h"
#include "NXPassShadowMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.
#include "NXLight.h"
#include "NXMaterial.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::OnMouseDown(NXEventArg eArg)
{
	auto ray = m_mainCamera->GenerateRay(Vector2(eArg.X, eArg.Y));
	printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
	shared_ptr<NXPrimitive> pHitPrimitive;
	Vector3 pHitPosition;
	float pHitDist;
	if (Intersect(ray, pHitPrimitive, pHitPosition, pHitDist))
	{
		printf("object: %s, hitPos: %.3f, %.3f, %.3f, dist: %.6f\n", pHitPrimitive->GetName().c_str(), pHitPosition.x, pHitPosition.y, pHitPosition.z, pHitDist);
	}
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
	pPointLight->SetTranslation(Vector3(1.0f, 1.0f, -1.0f));
	pPointLight->SetRange(100.0f);
	pPointLight->SetAtt(Vector3(0.0f, 0.0f, 1.0f));
	m_lights.push_back(pPointLight);

	auto pSpotLight = make_shared<NXSpotLight>();
	pSpotLight->SetAmbient(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
	pSpotLight->SetDiffuse(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pSpotLight->SetSpecular(Vector4(0.8f, 0.8f, 0.8f, 1.0f));
	pSpotLight->SetTranslation(Vector3(-1.0f, 1.0f, -1.0f));
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
	pMaterial->SetOpacity(0.2f);
	m_materials.push_back(pMaterial);

	auto pPlane = make_shared<NXPlane>();
	{
		pPlane->SetName("PlaneMir");
		pPlane->Init(5.0f, 5.0f);
		pPlane->SetMaterial(pMaterial);
		pPlane->SetTranslation(Vector3(0.0f, 0.0f, 0.0f));
		m_primitives.push_back(pPlane);
	}

	pPlane = make_shared<NXPlane>();
	{
		pPlane->SetName("Plane");
		pPlane->Init(5.0f, 5.0f);
		pPlane->SetMaterial(pMaterial);
		pPlane->SetTranslation(Vector3(0.0f, 2.5f, 2.5f));
		pPlane->SetRotation(Vector3(-XM_PIDIV2, 0.0f, 0.0f));
		m_primitives.push_back(pPlane);
	}

	//auto pScript_test = make_shared<NSTest>();
	//pPlane->AddScript(pScript_test);
	
	auto pSphere = make_shared<NXSphere>();
	{
		pSphere->SetName("Sphere");
		pSphere->Init(1.0f, 16, 16);
		pSphere->SetMaterial(pMaterial);
		pSphere->SetTranslation(Vector3(2.0f, 0.0f, 0.0f));
		m_primitives.push_back(pSphere);
	}

	auto pMesh = make_shared<NXMesh>();
	{
		pMesh->SetName("Mesh");
		pMesh->Init("D:\\test.fbx");
		pMesh->SetMaterial(pMaterial);
		pMesh->SetTranslation(Vector3(0.0f, 0.0f, 0.0f));
		m_primitives.push_back(pMesh);
	}

	auto pCamera = make_shared<NXCamera>();
	pCamera->Init(Vector3(0.0f, 0.0f, -1.5f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f));
	m_mainCamera = pCamera;

	auto pScript = make_shared<NSFirstPersonalCamera>();
	m_mainCamera->AddScript(pScript);

	auto pListener_onKeyDown = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	auto pListener_onKeyUp = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	auto pListener_onMouseMove = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventKeyDown::GetInstance()->AddListener(pListener_onKeyDown);
	NXEventKeyUp::GetInstance()->AddListener(pListener_onKeyUp);
	NXEventMouseMove::GetInstance()->AddListener(pListener_onMouseMove);

	auto pThisScene = dynamic_pointer_cast<Scene>(shared_from_this());
	auto pListener_onMouseDown = make_shared<NXListener>(pThisScene, std::bind(&Scene::OnMouseDown, pThisScene, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(pListener_onMouseDown);

	InitBoundingStructures();
}

void Scene::PrevUpdate()
{
	for (auto it = m_lights.begin(); it != m_lights.end(); it++)
	{
		(*it)->PrevUpdate();
	}

	m_mainCamera->PrevUpdate();

	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		(*it)->PrevUpdate();
	}
}

void Scene::UpdateScripts()
{
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		auto pPrim = *it;
		auto pPrimScripts = pPrim->GetScripts();
		for (auto itScripts = pPrimScripts.begin(); itScripts != pPrimScripts.end(); itScripts++)
		{
			auto pScript = *itScripts;
			pScript->Update();
		}
	}

	auto pScripts = m_mainCamera->GetScripts();
	for (auto itScripts = pScripts.begin(); itScripts != pScripts.end(); itScripts++)
	{
		auto pScript = *itScripts;
		pScript->Update();
	}
}

void Scene::UpdateCamera()
{
	m_mainCamera->Update();
}

void Scene::Release()
{
	if (m_cbLights) m_cbLights->Release();

	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		(*it)->Release();
	}

	if (m_mainCamera)
	{
		m_mainCamera->Release();
	}
}

void Scene::GetShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb)
{
	// 目前仅对第一个平行光提供支持
	Vector3 direction = dynamic_pointer_cast<NXDirectionalLight>(m_lights[0])->GetDirection();
	Vector3 shadowMapAt = m_boundingSphere.Center;
	Vector3 shadowMapEye = shadowMapAt - 2.0f * m_boundingSphere.Radius * direction;
	Vector3 shadowMapUp(0.0f, 1.0f, 0.0f);
	Matrix mxV = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

	Vector3 shadowMapAtInViewSpace = Vector3::Transform(shadowMapAt, mxV);
	Vector3 OrthoBoxRangeMin = shadowMapAtInViewSpace - Vector3(m_boundingSphere.Radius);
	Vector3 OrthoBoxRangeMax = shadowMapAtInViewSpace + Vector3(m_boundingSphere.Radius);
	Matrix mxP = XMMatrixOrthographicOffCenterLH(OrthoBoxRangeMin.x, OrthoBoxRangeMax.x, OrthoBoxRangeMin.y, OrthoBoxRangeMax.y, OrthoBoxRangeMin.z, OrthoBoxRangeMax.z);

	Matrix mxT(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	out_cb.view = mxV.Transpose();
	out_cb.projection = mxP.Transpose();
	out_cb.texture = mxT.Transpose();
}

void Scene::InitBoundingStructures()
{
	// construct AABB for scene.
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		AABB::CreateMerged(m_aabb, m_aabb, (*it)->GetAABB());
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

bool Scene::Intersect(const Ray& worldRay, shared_ptr<NXPrimitive>& outTarget, Vector3& outHitPosition, float& outDist)
{
	outTarget = nullptr;
	float minDist = FLT_MAX;
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		Ray LocalRay(
			Vector3::Transform(worldRay.position, (*it)->GetWorldMatrixInv()),
			Vector3::TransformNormal(worldRay.direction, (*it)->GetWorldMatrixInv())
		);
		LocalRay.direction.Normalize();

		// ray-aabb
		if (LocalRay.IntersectsFast((*it)->GetAABB(), outDist))
		{
			// ray-triangle
			if ((*it)->Intersect(LocalRay, outHitPosition, outDist))
			{
				if (minDist > outDist)
				{
					minDist = outDist;
					outTarget = *it;
				}
			}
		}
	}

	if (outTarget)
	{
		outHitPosition = Vector3::Transform(outHitPosition, outTarget->GetWorldMatrix());
		outDist = Vector3::Distance(worldRay.position, outHitPosition);
	}

	return outTarget != nullptr;
}
