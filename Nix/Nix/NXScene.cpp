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
#include "NXLight.h"
#include "NXMaterial.h"

//#include "NXShadowMap.h"
#include "NXPassShadowMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

Scene::Scene() :
	m_pRootObject(make_shared<NXObject>())
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
	m_sceneManager = make_shared<SceneManager>(dynamic_pointer_cast<Scene>(shared_from_this()));
	auto pDirLight = m_sceneManager->CreateDirectionalLight(
		"DirLight1",
		Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		Vector3(0.8f, 0.8f, 0.8f),
		1.0f,
		Vector3(1.0f, -1.0f, 1.0f)
		);

	auto pPointLight = m_sceneManager->CreatePointLight(
		"PointLight1",
		Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		Vector3(0.8f, 0.8f, 0.8f),
		1.0f,
		Vector3(1.0f, 1.0f, -1.0f),
		100.0f,
		Vector3(0.0f, 0.0f, 1.0f)
	);

	auto pSpotLight = m_sceneManager->CreateSpotLight(
		"SpotLight1",
		Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		Vector3(0.8f, 0.8f, 0.8f),
		1.0f,
		Vector3(1.0f, 1.0f, -1.0f),
		100.0f,
		Vector3(1.0f, -1.0f, 1.0f),
		1.0f,
		Vector3(0.0f, 0.0f, 1.0f)
	);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

	// 这里直接转指针很危险的，后面务必改一下lights的管理结构。
	m_cbDataLights.dirLight = pDirLight->GetLightInfo();
	m_cbDataLights.pointLight = pPointLight->GetLightInfo();
	m_cbDataLights.spotLight = pSpotLight->GetLightInfo();

	ConstantBufferLight cb;
	cb.dirLight = m_cbDataLights.dirLight;
	cb.pointLight = m_cbDataLights.pointLight;
	cb.spotLight = m_cbDataLights.spotLight;
	g_pContext->UpdateSubresource(m_cbLights, 0, nullptr, &cb, 0, 0);
	
	auto pMaterial = m_sceneManager->CreateMaterial(
		"defaultMaterial",
		Vector4(0.0f, 0.0f, 0.0f, 1.0f),
		Vector4(0.7f, 0.85f, 0.7f, 1.0f),
		Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		0.2f
	);

	auto pPlane = m_sceneManager->CreatePlane(
		"Wall",
		5.0f, 5.0f,
		pMaterial,
		Vector3(0.0f)
	);

	pPlane = m_sceneManager->CreatePlane(
		"Ground",
		5.0f, 5.0f,
		pMaterial,
		Vector3(0.0f, 2.5f, 2.5f),
		Vector3(-XM_PIDIV2, 0.0f, 0.0f)
	);
	
	auto pSphere = m_sceneManager->CreateSphere(
		"Sphere",
		1.0f, 16, 16,
		pMaterial,
		Vector3(2.0f, 0.0f, 0.0f)
	);

	vector<shared_ptr<NXMesh>> pMeshes;
	bool pMesh = m_sceneManager->CreateFBXMeshes(
		"D:\\2.fbx", 
		pMaterial,
		pMeshes
	);

	auto pCamera = m_sceneManager->CreateCamera(
		"Camera1", 
		0.01f, 1000.f, 
		Vector3(0.0f, 0.0f, -1.5f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	if (!pMeshes.empty())
	{
		bool bBind = m_sceneManager->BindParent(pMeshes[0], pSphere);
		if (bBind)
		{
			auto pScript_test = make_shared<NSTest>();
			pMeshes[0]->AddScript(pScript_test);
		}
	}

	m_mainCamera = pCamera;
	m_objects.push_back(pCamera);

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

	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。
	UpdateTransform(m_pRootObject);
	InitBoundingStructures();
}

void Scene::UpdateTransform(shared_ptr<NXObject> pObject)
{
	// pObject为空时代表从根节点开始更新Transform。
	if (!pObject)
	{
		UpdateTransform(m_pRootObject);
	}
	else
	{
		auto pT = dynamic_pointer_cast<NXTransform>(pObject);
		if (pT)
			pT->UpdateTransform();
		auto ch = pObject->GetChilds();
		for (auto it = ch.begin(); it != ch.end(); it++)
		{
			UpdateTransform(*it);
		}
	}
}

void Scene::UpdateScripts()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); it++)
	{
		auto scripts = (*it)->GetScripts();
		for (auto itScr = scripts.begin(); itScr != scripts.end(); itScr++)
		{
			auto pScript = *itScr;
			pScript->Update();
		}
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

	m_sceneManager.reset();
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
		AABB::CreateMerged(m_aabb, m_aabb, (*it)->GetAABBWorld());
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
		if (LocalRay.IntersectsFast((*it)->GetAABBLocal(), outDist))
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
