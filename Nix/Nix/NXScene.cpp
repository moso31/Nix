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
	m_sceneManager(new SceneManager(this)),
	m_cbLights(nullptr)
{
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArg eArg)
{
	auto ray = GetMainCamera()->GenerateRay(Vector2(eArg.X, eArg.Y));
	//printf("cursor: %.3f, %.3f\n", (float)eArg.X, (float)eArg.Y);
	//printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
}

void NXScene::OnKeyDown(NXEventArg eArg)
{
	switch (eArg.VKey)
	{
	case 'U':
		NXRayTracer::GetInstance().RenderImage(this, NXRayTraceRenderMode::DirectLighting);
		break;
	case 'T':
		NXRayTracer::GetInstance().RenderImage(this, NXRayTraceRenderMode::PathTracing);
		break;
	case 'Y':
		NXRayTracer::GetInstance().RenderImage(this, NXRayTraceRenderMode::PhotonMapping);
		break;
	case 'J':
		NXRayTracer::GetInstance().RenderImage(this, NXRayTraceRenderMode::IrradianceCache);
		break;
	case 'G':
		NXRayTracer::GetInstance().RenderImage(this, NXRayTraceRenderMode::SPPM);
		break;
	default:
		break;
	}

	if (eArg.VKey == 'H')
	{
		// �����󽻼��ٽṹ��������Ⱦ�ٶȡ�
		printf("Generating BVH Structure...");
		BuildBVHTrees(HBVHSplitMode::HLBVH);
		printf("done.\n");

		Vector2 sampleCoord = Vector2((float)200 * 0.5f, (float)150 * 0.5f);
		Ray rayWorld = GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)200, (float)150));

		NXHit hit;
		RayCast(rayWorld, hit);
		if (hit.pPrimitive)
		{
			hit.GenerateBSDF(true);
			RayCast(rayWorld, hit);
		}

		if (!GetPrimitives().empty())
		{
			auto pMainCamera = GetMainCamera();
			printf("camera: pos %f, %f, %f, at %f, %f, %f\n",
				pMainCamera->GetTranslation().x,
				pMainCamera->GetTranslation().y,
				pMainCamera->GetTranslation().z,
				pMainCamera->GetAt().x,
				pMainCamera->GetAt().y,
				pMainCamera->GetAt().z);
			printf("done.\n");
		}
	}
}

void NXScene::Init()
{
	NXVisibleTest::GetInstance().SetScene(this);

	NXPBRMaterial* pPBRMat[] = {
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.8f, 0.0f, 0.0f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.0f, 0.8f), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.6f, 0.0f), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(0.0f, 0.6f, 0.0f), 1.0f, 0.1f, 0.0f, 0.0f, 0.0f),
		m_sceneManager->CreatePBRMaterial(Vector3(1.0f), 0.0f, 0.0f, 1.0f, 1.0f, 1.55f),
		m_sceneManager->CreatePBRMaterial(Vector3(1.0f), 1.0f, 0.0f, 1.0f, 0.0f, 0.0f),
	};

	//pPBRMat[0]->SetTexAlbedo(L"D:\\NixAssets\\hex-stones1\\hex-stones1-albedo.png");
	pPBRMat[0]->SetTexAlbedo(L"D:\\test.png");

	//auto pPlane = m_sceneManager->CreatePlane("Ground", 8.0f, 12.0f, NXPlaneAxis(POSITIVE_Y), Vector3(0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +Y", 8.0f, 12.0f, NXPlaneAxis(NEGATIVE_Y), Vector3(0.0f, 6.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall -Z", 8.0f, 6.0f, NXPlaneAxis(POSITIVE_Z), Vector3(0.0f, 3.0f, -6.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +Z", 8.0f, 6.0f, NXPlaneAxis(NEGATIVE_Z), Vector3(0.0f, 3.0f, 6.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall -X", 6.0f, 12.0f, NXPlaneAxis(POSITIVE_X), Vector3(-4.0f, 3.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	//pPlane = m_sceneManager->CreatePlane("Wall +X", 6.0f, 12.0f, NXPlaneAxis(NEGATIVE_X), Vector3(4.0f, 3.0f, 0.0f));
	//pPlane->SetMaterialPBR(pPBRMat[0]);

	auto pBox = m_sceneManager->CreateBox("Box", 1.0f, 1.0f, 1.0f, Vector3(3.0f, 0.0f, 0.0f));
	pBox->SetMaterialPBR(pPBRMat[0]);

	auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 16, 16, Vector3(5.0f, 1.0f, 1.0f));
	pSphere->SetMaterialPBR(pPBRMat[0]);

	auto pCylinder = m_sceneManager->CreateCylinder("Cylinder", 1.0f, 3.0f, 16, 16, Vector3(-3.0f, 1.0f, 1.0f));
	pCylinder->SetMaterialPBR(pPBRMat[0]);

	//pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 16, 16, Vector3(-1.0f, 1.0f, 2.0f));
	//pSphere->SetMaterialPBR(pPBRMat[3]);

	//NXPlane* pLight = m_sceneManager->CreatePlane("Light", 2.0f, 2.0f, NXPlaneAxis(NEGATIVE_Y), Vector3(0.0f, 5.999f, 2.0f));
	//pLight->SetMaterialPBR(pPBRMat[0]);

	//float a[11] = { 0.001, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
	//Vector3 rBaseColor = Vector3(1.0, 0.782, 0.344);
	//int sz = 11;
	//for (int i = 0; i < sz; i++)
	//{
	//	float metallic = a[i];
	//	for(int j = 0; j < sz; j++)
	//	{
	//		Vector2 randomPos(i, j);
	//		auto pSphere = m_sceneManager->CreateSphere(
	//			"Sphere",
	//			0.5f, 16, 16,
	//			Vector3(randomPos.x, 0.5f, randomPos.y) * 1.2f
	//		);

	//		float roughness = a[j];
	//		auto m = m_sceneManager->CreatePBRMaterial(Vector3(1.0f, 0.782f, 0.344f), metallic, roughness, 0.0f, 0.0f, 0.0f);
	//		pSphere->SetMaterialPBR(m);
	//	}
	//}

	std::vector<NXMesh*> pMeshes;
	bool pMesh = m_sceneManager->CreateFBXMeshes(
		"D:\\2.fbx", 
		pPBRMat[0],
		pMeshes
	); 

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
	//	auto pScript_test = new NSTest();
	//	pMeshes[1]->AddScript(pScript_test);
	//}

	m_sceneManager->CreateCubeMap("Sky", L"D:\\sunsetcube1024.dds");

	// ����AABB��Ҫ�������꣬��Init�׶λ�û���õ��������꣬������Ҫ��ǰPrevUpdateһ�Ρ�
	UpdateTransform();
	InitBoundingStructures();

	// InitLights()
	{
		NXPBRPointLight* pPointLight;
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

	// ���ó������� 
	bool bEnableShadowMap = false;
	if (bEnableShadowMap)
	{
		InitShadowMapTransformInfo(NXGlobalBufferManager::m_cbDataShadowMap);
	}

	InitScripts();
}

void NXScene::InitScripts()
{
	auto pMainCamera = GetMainCamera();

	NSFirstPersonalCamera* pScript = dynamic_cast<NSFirstPersonalCamera*>(m_sceneManager->CreateScript(NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA, pMainCamera));
	//{
	//	NSFirstPersonalCamera* pScript2 = dynamic_cast<NSFirstPersonalCamera*>(new NSFirstPersonalCamera());
	//	pMainCamera->AddScript(pScript2);
	//}

	m_sceneManager->AddEventListener(NXEventType::NXEVENT_KEYDOWN, pMainCamera, std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	m_sceneManager->AddEventListener(NXEventType::NXEVENT_KEYUP, pMainCamera, std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	m_sceneManager->AddEventListener(NXEventType::NXEVENT_MOUSEUP, pMainCamera, std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));

	m_sceneManager->AddEventListener(NXEventType::NXEVENT_MOUSEDOWN, this, std::bind(&NXScene::OnMouseDown, this, std::placeholders::_1));
	m_sceneManager->AddEventListener(NXEventType::NXEVENT_KEYDOWN, this, std::bind(&NXScene::OnKeyDown, this, std::placeholders::_1));
}

void NXScene::UpdateTransform(NXObject* pObject)
{
	// ����pObject������Transform��ֵ�ͽű���
	// pObjectΪ��ʱ=���³�����ȫ�����塣
	if (!pObject)
	{
		UpdateTransform(m_sceneManager->m_pRootObject);
	}
	else
	{
		NXTransform* pT = dynamic_cast<NXTransform*>(pObject);
		if (pT) pT->UpdateTransform();

		auto ch = pObject->GetChilds();
		for (auto it = ch.begin(); it != ch.end(); it++)
		{
			UpdateTransform(*it);
		}
	}
}

void NXScene::UpdateScripts()
{
	for (auto it = m_sceneManager->m_objects.begin(); it != m_sceneManager->m_objects.end(); it++)
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
	GetMainCamera()->Update();
}

void NXScene::Release()
{
	if (m_cbLights)
		m_cbLights->Release();

	m_sceneManager->Release();
	delete m_sceneManager;
}

bool NXScene::RayCast(const Ray& ray, NXHit& outHitInfo, float tMax)
{
	outHitInfo.Reset();
	float outDist = tMax;

	auto pBVHTree = GetBVHTree();
	if (pBVHTree)
	{
		pBVHTree->Intersect(ray, outHitInfo, outDist);
	}
	else
	{
		for (auto prim : GetPrimitives())
		{
			// ray-aabb
			float aabbDist;
			if (ray.IntersectsFast(prim->GetAABBWorld(), aabbDist))
			{
				if (aabbDist < outDist)
				{
					// ray-triangle
					if (prim->RayCast(ray, outHitInfo, outDist))
					{
						// �õ��˸������ཻ�����
						// ������ǰoutHitInfo��outDist��
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
	auto lights = GetPBRLights();
	if (lights.empty())
		return;

	NXPBRDistantLight* pDistantLight = nullptr;
	for (auto pLight : lights)
	{
		pDistantLight = (NXPBRDistantLight*)(pLight);
		if (pDistantLight) break;  // Ŀǰ���Ե�һ��ƽ�й��ṩ֧��
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
	for (auto prim : GetPrimitives())
	{
		AABB::CreateMerged(m_aabb, m_aabb, prim->GetAABBWorld());
		prim->UpdateSurfaceAreaInfo();
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

void NXScene::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	m_sceneManager->BuildBVHTrees(SplitMode);
}
