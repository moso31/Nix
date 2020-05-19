#include "NXScene.h"
#include "SceneManager.h"
#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "NXIntersection.h"
//#include "HBVH.h"

#include "NXMesh.h"
#include "NXBox.h"
#include "NXSphere.h"
#include "NXCylinder.h"
#include "NXPlane.h"
#include "NXCamera.h"
#include "NXLight.h"
#include "NXMaterial.h"

#include "NXRayTracer.h"
#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

//#include "NXShadowMap.h"
#include "NXPassShadowMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

NXScene::NXScene() :
	m_pRootObject(make_shared<NXObject>()),
	m_cbLights(nullptr)
{
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArg eArg)
{
	auto ray = m_mainCamera->GenerateRay(Vector2(eArg.X, eArg.Y));
	printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
}

void NXScene::OnKeyDown(NXEventArg eArg)
{
	if (eArg.VKey == 'G')
	{
		printf("making...\n");

		auto pScene = dynamic_pointer_cast<NXScene>(shared_from_this());

		NXRenderImageInfo imageInfo;
		imageInfo.ImageSize = XMINT2(400, 300);
		imageInfo.EachPixelSamples = 1;
		shared_ptr<NXIntegrator> pWhitted = make_shared<NXIntegrator>();
		NXRayTracer::GetInstance()->MakeImage(pScene, m_mainCamera, pWhitted, imageInfo);

		printf("done.\n");
	}
}

void NXScene::Init()
{
	m_sceneManager = make_shared<SceneManager>(dynamic_pointer_cast<NXScene>(shared_from_this()));
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

	m_sceneManager->CreatePBRPointLight(Vector3(0.0f, 0.0f, 5.0f), Vector3(100.0f));

	auto pPBRMat = m_sceneManager->CreatePBRMatte(Vector3(1.0f, 0.0f, 0.0f), 1.0f);

	auto pPlane = m_sceneManager->CreatePlane(
		"Wall",
		5.0f, 5.0f,
		pMaterial,
		Vector3(0.0f)
	);

	pPlane->SetMaterialPBR(pPBRMat);

	pPlane = m_sceneManager->CreatePlane(
		"Ground",
		5.0f, 5.0f,
		pMaterial,
		Vector3(0.0f, 2.5f, 2.5f),
		Vector3(-XM_PIDIV2, 0.0f, 0.0f)
	);

	pPlane->SetMaterialPBR(pPBRMat);
	
	//auto pSphere = m_sceneManager->CreateSphere(
	//	"Sphere",
	//	1.0f, 16, 16,
	//	pMaterial,
	//	Vector3(2.0f, 0.0f, 0.0f)
	//);

	//pSphere->SetMaterialPBR(pPBRMat);

	vector<shared_ptr<NXMesh>> pMeshes;
	bool pMesh = m_sceneManager->CreateFBXMeshes(
		"D:\\2.fbx", 
		pMaterial,
		pMeshes
	);

	//pMeshes[0]->SetMaterialPBR(pPBRMat);

	auto pCamera = m_sceneManager->CreateCamera(
		"Camera1", 
		0.01f, 1000.f, 
		Vector3(0.0f, 0.0f, -1.5f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	//if (!pMeshes.empty())
	//{
	//	bool bBind = m_sceneManager->BindParent(pMeshes[1], pSphere);
	//	auto pScript_test = make_shared<NSTest>();
	//	pMeshes[1]->AddScript(pScript_test);
	//}

	m_mainCamera = pCamera;
	m_objects.push_back(pCamera);

	InitScripts();

	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。(nice annotation.)
	UpdateTransform(m_pRootObject);
	InitBoundingStructures();
	
	// 设置常量缓存
	InitShadowMapTransformInfo(NXGlobalBufferManager::m_cbDataShadowMap);
}

void NXScene::InitScripts()
{
	auto pScript = make_shared<NSFirstPersonalCamera>();
	m_mainCamera->AddScript(pScript);

	auto pListener_onKeyDown = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	auto pListener_onKeyUp = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	auto pListener_onMouseMove = make_shared<NXListener>(m_mainCamera, std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventKeyDown::GetInstance()->AddListener(pListener_onKeyDown);
	NXEventKeyUp::GetInstance()->AddListener(pListener_onKeyUp);
	NXEventMouseMove::GetInstance()->AddListener(pListener_onMouseMove);

	auto pThisScene = dynamic_pointer_cast<NXScene>(shared_from_this());
	auto pListener_onMouseDown = make_shared<NXListener>(pThisScene, std::bind(&NXScene::OnMouseDown, pThisScene, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(pListener_onMouseDown);
	pListener_onKeyDown = make_shared<NXListener>(pThisScene, std::bind(&NXScene::OnKeyDown, pThisScene, std::placeholders::_1));
	NXEventKeyDown::GetInstance()->AddListener(pListener_onKeyDown);
}

void NXScene::UpdateTransform(shared_ptr<NXObject> pObject)
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

void NXScene::UpdateScripts()
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

void NXScene::UpdateCamera()
{
	m_mainCamera->Update();
}

void NXScene::Release()
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

bool NXScene::RayCast(const Ray& ray, NXHit& outHitInfo)
{
	float outDist = FLT_MAX;

	// 目前还是用遍历找的……将来改成KD树或BVH树。
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		Ray LocalRay(
			Vector3::Transform(ray.position, (*it)->GetWorldMatrixInv()),
			Vector3::TransformNormal(ray.direction, (*it)->GetWorldMatrixInv())
		);
		LocalRay.direction.Normalize();

		// ray-aabb
		float aabbDist;
		if (LocalRay.IntersectsFast((*it)->GetAABBLocal(), aabbDist))
		{
			if (aabbDist < outDist)
			{
				// ray-triangle
				if ((*it)->RayCast(LocalRay, outHitInfo, outDist))
				{
					// 得到了更近的相交结果。
					// 保留当前outHitInfo和outDist。
				}
			}
		}
	}
	
	if (!outHitInfo.pPrimitive)
		return false;

	outHitInfo.LocalToWorld();
	return true;
}

void NXScene::InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb)
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

void NXScene::InitBoundingStructures()
{
	// construct AABB for scene.
	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		AABB::CreateMerged(m_aabb, m_aabb, (*it)->GetAABBWorld());
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}