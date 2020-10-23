#include "NXScene.h"
#include "SceneManager.h"
#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "NXIntersection.h"
#include "NXRandom.h"
//#include "HBVH.h"

#include "NXMesh.h"
#include "NXBox.h"
#include "NXSphere.h"
#include "NXCylinder.h"
#include "NXPlane.h"
#include "NXCamera.h"

#include "NXRayTracer.h"
#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

#include "NXCubeMap.h"

#include "NXPhoton.h"
#include "NXDirectIntegrator.h"
#include "NXPathIntegrator.h"
#include "NXPMIntegrator.h"
#include "NXPMSplitIntegrator.h"
#include "NXSPPMIntegrator.h"

//#include "NXShadowMap.h"
#include "NXPassShadowMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

NXScene::NXScene() :
	m_pRootObject(std::make_shared<NXObject>()),
	m_cbLights(nullptr)
{
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArg eArg)
{
	auto ray = m_pMainCamera->GenerateRay(Vector2(eArg.X, eArg.Y));
	//printf("cursor: %.3f, %.3f\n", (float)eArg.X, (float)eArg.Y);
	//printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
}

void NXScene::OnKeyDown(NXEventArg eArg)
{
	auto pScene = std::dynamic_pointer_cast<NXScene>(shared_from_this());

	switch (eArg.VKey)
	{
	case 'U':
		NXRayTracer::GetInstance()->RenderImage(pScene, NXRayTraceRenderMode::DirectLighting);
		break;
	case 'T':
		NXRayTracer::GetInstance()->RenderImage(pScene, NXRayTraceRenderMode::PathTracing);
		break;
	case 'Y':
		NXRayTracer::GetInstance()->RenderImage(pScene, NXRayTraceRenderMode::PhotonMapping);
		break;
	case 'J':
		NXRayTracer::GetInstance()->RenderImage(pScene, NXRayTraceRenderMode::IrradianceCache);
		break;
	case 'G':
		NXRayTracer::GetInstance()->RenderImage(pScene, NXRayTraceRenderMode::SPPM);
		break;
	default:
		break;
	}

	if (eArg.VKey == 'H')
	{
		// 创建求交加速结构以增加渲染速度。
		printf("Generating BVH Structure...");
		BuildBVHTrees(HLBVH);
		printf("done.\n");

		printf("center ray testing...\n");
		//NXRayTracer::GetInstance()->CenterRayTest(10);

		if (!m_primitives.empty())
		{
			std::shared_ptr<NXPrimitive> p = std::dynamic_pointer_cast<NXPrimitive>(m_primitives[0]);
			printf("camera: pos %f, %f, %f, at %f, %f, %f\n",
				m_pMainCamera->GetTranslation().x,
				m_pMainCamera->GetTranslation().y,
				m_pMainCamera->GetTranslation().z,
				m_pMainCamera->GetAt().x,
				m_pMainCamera->GetAt().y,
				m_pMainCamera->GetAt().z);
			printf("done.\n");
		}
	}
}

void NXScene::Init()
{
	auto pScene = std::dynamic_pointer_cast<NXScene>(shared_from_this());
	NXVisibleTest::GetInstance()->SetScene(pScene);

	m_sceneManager = std::make_shared<SceneManager>(std::dynamic_pointer_cast<NXScene>(shared_from_this()));
	std::shared_ptr<NXPBRMaterial> pPBRMat[] = {
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f, 0.0f, 0.0f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.0f, 0.8f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.6f, 0.0f), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.6f, 0.0f), 1.0f, 0.1f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(1.0f), 0.0f, 0.0f, 1.0f, 1.0f, 1.55f),
		m_sceneManager->CreatePBRMaterial(Vector3(1.0f), 1.0f, 0.0f, 1.0f, 0.0f, 0.0f),
	};

	//auto pPlane = m_sceneManager->CreatePlane("Ground", 8.0f, 12.0f, NXPlaneAxis(POSITIVE_Y), Vector3(0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +Y", 8.0f, 12.0f, NXPlaneAxis(NEGATIVE_Y), Vector3(0.0f, 6.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall -Z", 8.0f, 6.0f, NXPlaneAxis(POSITIVE_Z), Vector3(0.0f, 3.0f, -6.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +Z", 8.0f, 6.0f, NXPlaneAxis(NEGATIVE_Z), Vector3(0.0f, 3.0f, 6.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall -X", 6.0f, 12.0f, NXPlaneAxis(POSITIVE_X), Vector3(-4.0f, 3.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[1]);

	//pPlane = m_sceneManager->CreatePlane("Wall +X", 6.0f, 12.0f, NXPlaneAxis(NEGATIVE_X), Vector3(4.0f, 3.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[2]);

	//auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 16, 16, Vector3(1.0f, 1.0f, 1.0f));
	//pSphere->SetMaterialPBR(pPBRMat[4]);

	//pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 16, 16, Vector3(-1.0f, 1.0f, 2.0f));
	//pSphere->SetMaterialPBR(pPBRMat[3]);

	//std::shared_ptr<NXPlane> pLight = m_sceneManager->CreatePlane("Light", 2.0f, 2.0f, NXPlaneAxis(NEGATIVE_Y), Vector3(0.0f, 5.999f, 2.0f));
	//pLight->SetMaterialPBR(pPBRMat[0]);

	float a[11] = { 0.001, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
	Vector3 rBaseColor = Vector3(1.0, 0.782, 0.344);
	int sz = 11;
	for (int i = 0; i < sz; i++)
	{
		float metallic = a[i];
		for(int j = 0; j < sz; j++)
		{
			Vector2 randomPos(i, j);
			auto pSphere = m_sceneManager->CreateSphere(
				"Sphere",
				0.5f, 16, 16,
				Vector3(randomPos.x, 0.5f, randomPos.y) * 1.2f
			);

			float roughness = a[j];
			auto m = m_sceneManager->CreatePBRMaterial(Vector3(1.0f, 0.782f, 0.344f), metallic, roughness, 0.0f, 0.0f, 0.0f);
			pSphere->SetMaterialPBR(m);
		}
	}

	//vector<std::shared_ptr<NXMesh>> pMeshes;
	//bool pMesh = m_sceneManager->CreateFBXMeshes(
	//	"D:\\2.fbx", 
	//	pMaterial,
	//	pMeshes
	//);

	//pMeshes[0]->SetMaterialPBR(pPBRMat);
	//pMeshes[1]->SetMaterialPBR(pPBRMat);

	auto pCamera = m_sceneManager->CreateCamera(
		"Camera1",
		70.0f, 0.01f, 1000.f,
		//Vector3(0.0f, 1.0f, 0.0f),
		//Vector3(0.0f, 0.0f, 0.0001f),
		Vector3(0.0f, 3.0f, -5.0f),
		Vector3(0.0f, 3.0f, 0.0f),
		//Vector3(0.201792, 0.895896, -1.904944),
		//Vector3(0.201792, 0.895896, -0.904944),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	//if (!pMeshes.empty())
	//{
	//	bool bBind = m_sceneManager->BindParent(pMeshes[1], pSphere);
	//	auto pScript_test = std::make_shared<NSTest>();
	//	pMeshes[1]->AddScript(pScript_test);
	//}

	m_pMainCamera = pCamera;
	m_objects.push_back(pCamera);

	m_pCubeMap = std::make_shared<NXCubeMap>();
	m_pCubeMap->Init(L"D:\\sunsetcube1024.dds");

	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。
	UpdateTransform(m_pRootObject);
	InitBoundingStructures();

	// InitLights()
	{
		std::shared_ptr<NXPBRPointLight> pPointLight;
		pPointLight = m_sceneManager->CreatePBRPointLight(Vector3(0.0f, 4.5f, 0.0f), Vector3(20.0f));
		//m_sceneManager->CreatePBRDistantLight(Vector3(-1.0f, 0.0f, 1.0f), Vector3(2.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight,  Vector3(20.0f)); 
		//m_sceneManager->CreatePBREnvironmentLight(m_pCubeMap, Vector3(1.0f));

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

		m_cbDataLights.pointLight = pPointLight->GetConstantBuffer();
		g_pContext->UpdateSubresource(m_cbLights, 0, nullptr, &m_cbDataLights.pointLight, 0, 0);
	}

	// 设置常量缓存 
	bool bEnableShadowMap = false;
	if (bEnableShadowMap)
	{
		InitShadowMapTransformInfo(NXGlobalBufferManager::m_cbDataShadowMap);
	}

	InitScripts();
}

void NXScene::InitScripts()
{
	auto pScript = std::make_shared<NSFirstPersonalCamera>();
	m_pMainCamera->AddScript(pScript);

	auto pListener_onKeyDown = std::make_shared<NXListener>(m_pMainCamera, std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	auto pListener_onKeyUp = std::make_shared<NXListener>(m_pMainCamera, std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	auto pListener_onMouseMove = std::make_shared<NXListener>(m_pMainCamera, std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventKeyDown::GetInstance()->AddListener(pListener_onKeyDown);
	NXEventKeyUp::GetInstance()->AddListener(pListener_onKeyUp);
	NXEventMouseMove::GetInstance()->AddListener(pListener_onMouseMove);

	auto pThisScene = std::dynamic_pointer_cast<NXScene>(shared_from_this());
	auto pListener_onMouseDown = std::make_shared<NXListener>(pThisScene, std::bind(&NXScene::OnMouseDown, pThisScene, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(pListener_onMouseDown);
	pListener_onKeyDown = std::make_shared<NXListener>(pThisScene, std::bind(&NXScene::OnKeyDown, pThisScene, std::placeholders::_1));
	NXEventKeyDown::GetInstance()->AddListener(pListener_onKeyDown);
}

void NXScene::UpdateTransform(std::shared_ptr<NXObject> pObject)
{
	// pObject为空时代表从根节点开始更新Transform。
	if (!pObject)
	{
		UpdateTransform(m_pRootObject);
	}
	else
	{
		auto pT = std::dynamic_pointer_cast<NXTransform>(pObject);
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
	m_pMainCamera->Update();
}

void NXScene::Release()
{
	if (m_cbLights) m_cbLights->Release();

	for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
	{
		(*it)->Release();
	}

	if (m_pMainCamera)
	{
		m_pMainCamera->Release();
	}

	if (m_pCubeMap)
	{
		m_pCubeMap->Release();
		m_pCubeMap.reset();
	}

	m_sceneManager.reset();
	m_pBVHTree.reset();
}

bool NXScene::RayCast(const Ray& ray, NXHit& outHitInfo, float tMax)
{
	outHitInfo.Reset();
	float outDist = tMax;

	if (m_pBVHTree)
	{
		m_pBVHTree->Intersect(ray, outHitInfo, outDist);
	}
	else
	{
		for (auto it = m_primitives.begin(); it != m_primitives.end(); it++)
		{
			// ray-aabb
			float aabbDist;
			if (ray.IntersectsFast((*it)->GetAABBWorld(), aabbDist))
			{
				if (aabbDist < outDist)
				{
					// ray-triangle
					if ((*it)->RayCast(ray, outHitInfo, outDist))
					{
						// 得到了更近的相交结果。
						// 保留当前outHitInfo和outDist。
					}
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
	if (m_lights.empty())
		return;

	std::shared_ptr<NXPBRDistantLight> pDistantLight = nullptr;
	for (auto pLight : m_lights)
	{
		pDistantLight = std::dynamic_pointer_cast<NXPBRDistantLight>(pLight);
		if (pDistantLight) break;  // 目前仅对第一个平行光提供支持
	}

	Vector3 shadowMapAt = m_boundingSphere.Center;
	Vector3 shadowMapEye = shadowMapAt - 2.0f * m_boundingSphere.Radius * pDistantLight->Direction;
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
		(*it)->UpdateSurfaceAreaInfo();
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

void NXScene::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	if (m_pBVHTree)
	{
		m_pBVHTree->Release();
		m_pBVHTree.reset();
	}

	auto pThis = std::dynamic_pointer_cast<NXScene>(shared_from_this());
	m_pBVHTree = std::make_shared<HBVHTree>(pThis, m_primitives);
	m_pBVHTree->BuildTreesWithScene(SplitMode);
}
