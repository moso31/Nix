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
#include "NXLight.h"
#include "NXMaterial.h"

#include "NXRayTracer.h"
#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

#include "NXCubeMap.h"

#include "NXPhoton.h"
#include "NXDirectIntegrator.h"
#include "NXPathIntegrator.h"
#include "NXPMIntegrator.h"
#include "NXPMSplitIntegrator.h"

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
	auto pScene = dynamic_pointer_cast<NXScene>(shared_from_this());
	NXRenderImageInfo imageInfo;
	imageInfo.ImageSize = XMINT2(800, 600);
	imageInfo.EachPixelSamples = 4;

	if (eArg.VKey == 'G')
	{
		imageInfo.outPath = "D:\\nix_PMtracing.bmp";
		printf("rendering(HLBVH)...\n");
		CreateBVHTrees(HBVHSplitMode::HLBVH);

		shared_ptr<NXPhotonMap> pGlobalPhotonMap = make_shared<NXPhotonMap>(200000);
		pGlobalPhotonMap->Generate(pScene, m_mainCamera, PhotonMapType::Global);
		shared_ptr<NXPhotonMap> pCausticPhotonMap = make_shared<NXPhotonMap>(200000);
		pCausticPhotonMap->Generate(pScene, m_mainCamera, PhotonMapType::Caustic);

		//shared_ptr<NXPMIntegrator> pIntegrator = make_shared<NXPMIntegrator>(pGlobalPhotonMap);
		shared_ptr<NXPMSplitIntegrator> pIntegrator = make_shared<NXPMSplitIntegrator>(pGlobalPhotonMap, pCausticPhotonMap);

		NXRayTracer::GetInstance()->Load(pScene, m_mainCamera, pIntegrator, imageInfo);
		//NXRayTracer::GetInstance()->MakeIrradianceCache();
		NXRayTracer::GetInstance()->MakeImage();

		pIntegrator.reset();
	}

	if (eArg.VKey == 'T')
	{
		shared_ptr<NXIntegrator> pIntegrator = make_shared<NXPathIntegrator>();

		imageInfo.outPath = "D:\\nix_pathtracing.bmp";
		printf("rendering(HLBVH)...\n");
		CreateBVHTrees(HBVHSplitMode::HLBVH);
		NXRayTracer::GetInstance()->Load(pScene, m_mainCamera, pIntegrator, imageInfo);
		NXRayTracer::GetInstance()->MakeImage();

		pIntegrator.reset();
	}

	if (eArg.VKey == 'U')
	{
		shared_ptr<NXIntegrator> pIntegrator = make_shared<NXDirectIntegrator>();

		imageInfo.outPath = "D:\\nix_directlighting.bmp";
		printf("rendering(HLBVH)...\n");
		CreateBVHTrees(HBVHSplitMode::HLBVH);
		NXRayTracer::GetInstance()->Load(pScene, m_mainCamera, pIntegrator, imageInfo);
		NXRayTracer::GetInstance()->MakeImage();

		pIntegrator.reset();
	}

	if (eArg.VKey == 'H')
	{
		// 创建求交加速结构以增加渲染速度。
		printf("Generating BVH Structure...");
		CreateBVHTrees(HBVHSplitMode::HLBVH);
		printf("done.\n");

		shared_ptr<NXIntegrator> pIntegrator = make_shared<NXDirectIntegrator>();
		//shared_ptr<NXIntegrator> pIntegrator = make_shared<NXPathIntegrator>();
		//shared_ptr<NXPMIntegrator> pIntegrator = make_shared<NXPMIntegrator>();
		//shared_ptr<NXPMSplitIntegrator> pIntegrator = make_shared<NXPMSplitIntegrator>();
		//auto pThis = dynamic_pointer_cast<NXScene>(shared_from_this());
		//pIntegrator->GeneratePhotons(pThis, m_mainCamera);

		printf("center ray testing...\n");
		NXRayTracer::GetInstance()->Load(pScene, m_mainCamera, pIntegrator, imageInfo);
		NXRayTracer::GetInstance()->CenterRayTest(10);

		if (!m_primitives.empty())
		{
			shared_ptr<NXPrimitive> p = dynamic_pointer_cast<NXPrimitive>(m_primitives[0]);
			printf("camera: pos %f, %f, %f, at %f, %f, %f\n",
				m_mainCamera->GetTranslation().x,
				m_mainCamera->GetTranslation().y,
				m_mainCamera->GetTranslation().z,
				m_mainCamera->GetAt().x,
				m_mainCamera->GetAt().y,
				m_mainCamera->GetAt().z);
			printf("done.\n");
		}

		pIntegrator.reset();
	}
}

void NXScene::Init()
{
	auto pScene = dynamic_pointer_cast<NXScene>(shared_from_this());
	NXVisibleTest::GetInstance()->SetScene(pScene);

	m_sceneManager = make_shared<SceneManager>(dynamic_pointer_cast<NXScene>(shared_from_this()));
	auto pMaterial = m_sceneManager->CreateMaterial(
		"defaultMaterial",
		Vector4(0.0f, 0.0f, 0.0f, 1.0f),
		Vector4(0.7f, 0.85f, 0.7f, 1.0f),
		Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		0.2f
	);

	shared_ptr<NXPBRMaterial> pPBRMat[] = {
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f), Vector3(0.0f), Vector3(0.0f), Vector3(0.0f), 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f, 0.0f, 0.0f), Vector3(0.0f), Vector3(0.0f), Vector3(0.0f), 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.0f, 0.8f), Vector3(0.0f), Vector3(0.0f), Vector3(0.0f), 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f), Vector3(0.0f), 0.03f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), Vector3(1.0f), 0.5f, 1.55f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), Vector3(0.0f), 0.0f, 1.55f),
	};

	auto pPlane = m_sceneManager->CreatePlane("Ground", 8.0f, 12.0f, NXPlaneAxis(POSITIVE_Y), pMaterial, Vector3(0.0f));
	pPlane->SetMaterialPBR(pPBRMat[0]);

	pPlane = m_sceneManager->CreatePlane("Wall +Y", 8.0f, 12.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(0.0f, 6.0f, 0.0f));
	pPlane->SetMaterialPBR(pPBRMat[0]);

	pPlane = m_sceneManager->CreatePlane("Wall -Z", 8.0f, 6.0f, NXPlaneAxis(POSITIVE_Z), pMaterial, Vector3(0.0f, 3.0f, -6.0f));
	pPlane->SetMaterialPBR(pPBRMat[0]);

	pPlane = m_sceneManager->CreatePlane("Wall +Z", 8.0f, 6.0f, NXPlaneAxis(NEGATIVE_Z), pMaterial, Vector3(0.0f, 3.0f, 6.0f));
	pPlane->SetMaterialPBR(pPBRMat[0]);

	pPlane = m_sceneManager->CreatePlane("Wall -X", 6.0f, 12.0f, NXPlaneAxis(POSITIVE_X), pMaterial, Vector3(-4.0f, 3.0f, 0.0f));
	pPlane->SetMaterialPBR(pPBRMat[1]);

	pPlane = m_sceneManager->CreatePlane("Wall +X", 6.0f, 12.0f, NXPlaneAxis(NEGATIVE_X), pMaterial, Vector3(4.0f, 3.0f, 0.0f));
	pPlane->SetMaterialPBR(pPBRMat[2]);

	auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 16, 16, pMaterial, Vector3(1.0f, 1.0f, 1.0f));
	pSphere->SetMaterialPBR(pPBRMat[4]);

	//shared_ptr<NXSphere> pLight = m_sceneManager->CreateSphere("Light", 1.0f, 64, 64, pMaterial, Vector3(-4.5f, 5.999f, 2.0f));
	//pLight->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXSphere> pLight1 = m_sceneManager->CreateSphere("Light", 0.75f, 64, 64, pMaterial, Vector3(-1.5f, 5.999f, 2.0f));
	//pLight1->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXSphere> pLight2 = m_sceneManager->CreateSphere("Light", 0.5f, 64, 64, pMaterial, Vector3(1.5f, 5.999f, 2.0f));
	//pLight2->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXSphere> pLight3 = m_sceneManager->CreateSphere("Light", 0.25f, 64, 64, pMaterial, Vector3(4.5f, 5.999f, 2.0f));
	//pLight3->SetMaterialPBR(pPBRMat[0]);

	shared_ptr<NXPlane> pLight = m_sceneManager->CreatePlane("Light", 2.0f, 2.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(0.0f, 5.999f, 2.0f));
	pLight->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight = m_sceneManager->CreatePlane("Light", 1.0f, 1.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(0.0f, 5.999f, 2.0f));
	//pLight->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight1 = m_sceneManager->CreatePlane("Light", 1.0f, 1.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(-1.5f, 5.999f, 2.0f));
	//pLight1->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight2 = m_sceneManager->CreatePlane("Light", 1.0f, 1.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(1.5f, 5.999f, 2.0f));
	//pLight2->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight3 = m_sceneManager->CreatePlane("Light", 1.0f, 1.0f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(4.5f, 5.999f, 2.0f));
	//pLight3->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight1 = m_sceneManager->CreatePlane("Light", 0.75f, 0.75f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(-1.5f, 5.999f, 2.0f));
	//pLight1->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight2 = m_sceneManager->CreatePlane("Light", 0.5f, 0.5f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(1.5f, 5.999f, 2.0f));
	//pLight2->SetMaterialPBR(pPBRMat[0]);

	//shared_ptr<NXPlane> pLight3 = m_sceneManager->CreatePlane("Light", 0.25f, 0.25f, NXPlaneAxis(NEGATIVE_Y), pMaterial, Vector3(4.5f, 5.999f, 2.0f));
	//pLight3->SetMaterialPBR(pPBRMat[0]);

	//float a[4] = { 0.216, 0.036, 0.006, 0.001 };
	//float b[4] = { 27.5f, 40.0f, 52.5f, 65.0f };
	//for (int i = 1; i < 5; i++)
	//{
	//	float angle = XM_PIDIV2 * i / 6.0f;
	//	auto pPlane = m_sceneManager->CreatePlane("Grounds", 10.0f, 1.0f, NXPlaneAxis(POSITIVE_Y), pMaterial, 6.0f * Vector3(0.0f, 1.0 - cosf(angle), sinf(angle)), Vector3(-b[i - 1] * XM_PI / 180.0f, 0.0f, 0.0f));
	//	pPlane->SetMaterialPBR(m_sceneManager->CreatePBRMaterial(Vector3(0.5f), Vector3(0.5f), Vector3(0.0f), Vector3(0.0f), a[i - 1], 0.0f));
	//}

	//auto pPlane = m_sceneManager->CreatePlane("Ground", 50.0f, 12.0f, NXPlaneAxis(POSITIVE_Y), pMaterial, Vector3(0.0f, -5.0f, -3.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +Z", 50.0f, 50.0f, NXPlaneAxis(NEGATIVE_Z), pMaterial, Vector3(0.0f, 20.0f, 3.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//float a[11] = { 0.001, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
	//Vector3 rBaseColor = Vector3(1.0, 0.782, 0.344);
	//int sz = 11;
	//for (int i = 0; i < sz; i++)
	//{
	//	float metalness = a[i];
	//	for(int j = 0; j < sz; j++)
	//	{
	//		Vector2 randomPos(i, j);
	//		auto pSphere = m_sceneManager->CreateSphere(
	//			"Sphere",
	//			0.5f, 16, 16,
	//			pMaterial,
	//			Vector3(randomPos.x, 0.5f, randomPos.y) * 1.2f
	//		);

	//		float roughness = a[j];
	//		auto m = m_sceneManager->CreatePBRMaterial(Vector3(a[i]), Vector3(1.0f - a[j]), roughness);
	//		pSphere->SetMaterialPBR(m);
	//	}
	//}

	//vector<shared_ptr<NXMesh>> pMeshes;
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

	m_pCubeMap = make_shared<NXCubeMap>();
	m_pCubeMap->Init(L"D:\\sunsetcube1024.dds");

	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。
	UpdateTransform(m_pRootObject);
	InitBoundingStructures();

	// InitLights()
	{
		//m_sceneManager->CreatePBRPointLight(Vector3(0.0f, 4.5f, 0.0f), Vector3(1.0f));
		//m_sceneManager->CreatePBRDistantLight(Vector3(-1.0f), Vector3(1.0f));
		m_sceneManager->CreatePBRTangibleLight(pLight,  Vector3(10.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight1, Vector3(20.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight2, Vector3(20.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight3, Vector3(20.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight1, Vector3( 88.9f));
		//m_sceneManager->CreatePBRTangibleLight(pLight2, Vector3(200.0f));
		//m_sceneManager->CreatePBRTangibleLight(pLight3, Vector3(800.0f));
		//m_sceneManager->CreatePBREnvironmentLight(m_pCubeMap, Vector3(1.0f));

		//auto pDirLight = m_sceneManager->CreateDirectionalLight(
		//	"DirLight1",
		//	Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		//	Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		//	Vector3(0.8f, 0.8f, 0.8f),
		//	1.0f,
		//	Vector3(1.0f, -1.0f, 1.0f)
		//);

		//auto pPointLight = m_sceneManager->CreatePointLight(
		//	"PointLight1",
		//	Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		//	Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		//	Vector3(0.8f, 0.8f, 0.8f),
		//	1.0f,
		//	Vector3(0.0f, 1.0f, 0.0f),
		//	100.0f,
		//	Vector3(0.0f, 0.0f, 1.0f)
		//);

		//auto pSpotLight = m_sceneManager->CreateSpotLight(
		//	"SpotLight1",
		//	Vector4(0.2f, 0.2f, 0.2f, 1.0f),
		//	Vector4(0.8f, 0.8f, 0.8f, 1.0f),
		//	Vector3(0.8f, 0.8f, 0.8f),
		//	1.0f,
		//	Vector3(0.0f, 2.5f, 0.0f),
		//	100.0f,
		//	Vector3(1.0f, -1.0f, 1.0f),
		//	1.0f,
		//	Vector3(0.0f, 0.0f, 1.0f)
		//);

		//D3D11_BUFFER_DESC bufferDesc;
		//ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		//bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
		//bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		//bufferDesc.CPUAccessFlags = 0;
		//NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

		//// 这里直接转指针很危险的，后面务必改一下lights的管理结构。
		//m_cbDataLights.dirLight = pDirLight->GetLightInfo();
		//m_cbDataLights.pointLight = pPointLight->GetLightInfo();
		//m_cbDataLights.spotLight = pSpotLight->GetLightInfo();

		//ConstantBufferLight cb;
		//cb.dirLight = m_cbDataLights.dirLight;
		//cb.pointLight = m_cbDataLights.pointLight;
		//cb.spotLight = m_cbDataLights.spotLight;
		//g_pContext->UpdateSubresource(m_cbLights, 0, nullptr, &cb, 0, 0);
	}

	// 设置常量缓存
	InitShadowMapTransformInfo(NXGlobalBufferManager::m_cbDataShadowMap);

	InitScripts();
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
		(*it)->UpdateSurfaceAreaInfo();
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

void NXScene::CreateBVHTrees(const HBVHSplitMode SplitMode)
{
	if (m_pBVHTree)
	{
		m_pBVHTree->Release();
		m_pBVHTree.reset();
	}

	auto pThis = dynamic_pointer_cast<NXScene>(shared_from_this());
	m_pBVHTree = make_shared<HBVHTree>(pThis, m_primitives);
	m_pBVHTree->BuildTreesWithScene(SplitMode);
}
