#include "NXScene.h"
#include "SceneManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "NXIntersection.h"
#include "NXRandom.h"
//#include "HBVH.h"

#include "NXPrimitive.h"
#include "NXCamera.h"

#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

#include "NXCubeMap.h"

//#include "NXShadowMap.h"
#include "NXPassShadowMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

NXScene::NXScene() :
	m_sceneManager(new SceneManager(this)),
	m_pPickingObject(nullptr)
{
	m_type = NXType::eScene;
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArgMouse eArg)
{
	auto ray = GetMainCamera()->GenerateRay(Vector2(eArg.X + 0.5f, eArg.Y + 0.5f));
	//printf("cursor: %.3f, %.3f\n", (float)eArg.X, (float)eArg.Y);
	//printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);

	NXHit hit;
	RayCast(ray, hit);
	if (hit.pSubMesh)
	{
		SetCurrentPickingSubMesh(hit.pSubMesh);
	}
}

void NXScene::OnKeyDown(NXEventArgKey eArg)
{
	if (eArg.VKey == 'H')
	{
		// 创建求交加速结构以增加渲染速度。
		printf("Generating BVH Structure...");
		BuildBVHTrees(HBVHSplitMode::HLBVH);
		printf("done.\n");

		Vector2 sampleCoord = Vector2((float)200 * 0.5f, (float)150 * 0.5f);
		Ray rayWorld = GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)200, (float)150));

		NXHit hit;
		RayCast(rayWorld, hit);
		if (hit.pSubMesh)
		{
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
	NXPBRMaterial* pPBRMat[] = {
		m_sceneManager->CreatePBRMaterial("rustediron2", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		m_sceneManager->CreatePBRMaterial("hex-stones1", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		m_sceneManager->CreatePBRMaterial("pirate-gold", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		m_sceneManager->CreatePBRMaterial("circle-textured-metal1", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
	};

	pPBRMat[0]->SetTexAlbedo(L"D:\\NixAssets\\rustediron2\\albedo.png", true);
	pPBRMat[0]->SetTexNormal(L"D:\\NixAssets\\rustediron2\\normal.png", true);
	pPBRMat[0]->SetTexMetallic(L"D:\\NixAssets\\rustediron2\\metallic.png", true);
	pPBRMat[0]->SetTexRoughness(L"D:\\NixAssets\\rustediron2\\roughness.png", true);
	pPBRMat[0]->SetTexAO(L"D:\\NixAssets\\rustediron2\\ao.png", true);

	pPBRMat[1]->SetTexAlbedo(L"D:\\NixAssets\\hex-stones1\\albedo.png", true);
	pPBRMat[1]->SetTexNormal(L"D:\\NixAssets\\hex-stones1\\normal.png", true);
	pPBRMat[1]->SetTexMetallic(L"D:\\NixAssets\\hex-stones1\\metallic.png", true);
	pPBRMat[1]->SetTexRoughness(L"D:\\NixAssets\\hex-stones1\\roughness.png", true);
	pPBRMat[1]->SetTexAO(L"D:\\NixAssets\\hex-stones1\\ao.png", true);

	pPBRMat[2]->SetTexAlbedo(L"D:\\NixAssets\\pirate-gold\\albedo.png", true);
	pPBRMat[2]->SetTexNormal(L"D:\\NixAssets\\pirate-gold\\normal.png", true);
	pPBRMat[2]->SetTexMetallic(L"D:\\NixAssets\\pirate-gold\\metallic.png", true);
	pPBRMat[2]->SetTexRoughness(L"D:\\NixAssets\\pirate-gold\\roughness.png", true);
	pPBRMat[2]->SetTexAO(L"D:\\NixAssets\\pirate-gold\\ao.png", true);

	pPBRMat[3]->SetTexAlbedo(L"D:\\NixAssets\\circle-textured-metal1\\albedo.png", true);
	pPBRMat[3]->SetTexNormal(L"D:\\NixAssets\\circle-textured-metal1\\normal.png", true);
	pPBRMat[3]->SetTexMetallic(L"D:\\NixAssets\\circle-textured-metal1\\metallic.png", true);
	pPBRMat[3]->SetTexRoughness(L"D:\\NixAssets\\circle-textured-metal1\\roughness.png", true);
	pPBRMat[3]->SetTexAO(L"D:\\NixAssets\\circle-textured-metal1\\ao.png", true);

	//auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 64, 64);
	//pSphere->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);

	for (int l = 0; l < 4; l++)
	{
		for (int m = -l; m <= l; m++)
		{
			Vector3 objPos(m * 1.5f, -l * 1.5f, 0.0f);
			auto pSH = m_sceneManager->CreateSHSphere("shTest", l, m, 1.0f, 64, 64, objPos);
			pSH->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
			pSH->AddScript(new NSTest());
		}
	}

	//auto p = m_sceneManager->CreatePlane("Sphere", 10.0f, 10.0f, NXPlaneAxis::POSITIVE_Y);
	//p->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	
	//std::vector<NXPrimitive*> pMeshes;
	//m_sceneManager->CreateFBXMeshes("D:\\NixAssets\\UnityBall.fbx", pMeshes);
	//pMeshes[0]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[0]->GetSubMesh(1)->SetMaterialPBR(pPBRMat[1]);
	//pMeshes[0]->GetSubMesh(2)->SetMaterialPBR(pPBRMat[3]);
	//pMeshes[0]->GetSubMesh(3)->SetMaterialPBR(pPBRMat[3]);
	//pMeshes[0]->SetRotation(Vector3(-0.8f, 0.0f, 0.0f));

	//for (int i = -5; i <= 5; i++)
	//{
	//	NXPBRMaterial* pPBRMat = m_sceneManager->CreatePBRMaterial("rustediron2", Vector3(1.0f), Vector3(1.0f), 1.0f, i * 0.1f + 0.5f, 1.0f);
	//	auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 64, 64, Vector3(i * 2.0f, 0.0f, 0.0f));
	//	pSphere->GetSubMesh(0)->SetMaterialPBR(pPBRMat);

	//	pPBRMat = m_sceneManager->CreatePBRMaterial("rustediron2", Vector3(1.0f), Vector3(1.0f), 0.0f, i * 0.1f + 0.5f, 1.0f);
	//	pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 64, 64, Vector3(i * 2.0f, 2.0f, 0.0f));
	//	pSphere->GetSubMesh(0)->SetMaterialPBR(pPBRMat);
	//}

	//m_sceneManager->CreateFBXMeshes("D:\\NixAssets\\Cloth.fbx", pMeshes, true);
	//pMeshes[0]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[1]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[2]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[3]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[4]->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//pMeshes[0]->SetScale(Vector3(0.1f, 0.1f, 0.1f));
	//pMeshes[1]->SetScale(Vector3(0.1f, 0.1f, 0.1f));
	//pMeshes[2]->SetScale(Vector3(0.1f, 0.1f, 0.1f));
	//pMeshes[3]->SetScale(Vector3(0.1f, 0.1f, 0.1f));
	//pMeshes[4]->SetScale(Vector3(0.1f, 0.1f, 0.1f));

	{
		//bool bBind = m_sceneManager->BindParent(pMeshes[1], pSphere);
		//auto pScript_test = new NSTest();
		//pMeshes[0]->AddScript(pScript_test);
	}

	// 设置Picking Object（Demo用，临时）
	//SetCurrentPickingSubMesh(pSphere->GetSubMesh(0));

	//for (float i = -8.0f; i < 8.01f; i += 2.0f)
	//{
	//	for (float j = -8.0f; j < 8.01f; j += 2.0f)
	//	{
	//		Vector3 pos(i, 0, j);
	//		pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 64, 64, pos);
	//			pSphere->GetSubMesh(0)->SetMaterialPBR(pPBRMat[0]);
	//	}
	//}

	auto pCamera = m_sceneManager->CreateCamera(
		"Camera1",
		70.0f, 0.3f, 1000.f,
		Vector3(0.0f, 5.0f, 7.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);


	auto pMainCamera = GetMainCamera();
	pMainCamera->SetTranslation(Vector3(0.0f, 1.0f, -5.0f));
	pMainCamera->SetLookAt(Vector3(0.0f, 1.0f, -4.0f));

	m_sceneManager->CreateCubeMap("Sky", L"D:\\Alexs_Apt_2k.hdr");
	//m_sceneManager->CreateCubeMap("Sky", L"D:\\TexturesCom_JapanInariTempleH_1K_hdri_sphere.hdr");
	//m_sceneManager->CreateCubeMap("Sky", L"D:\\ballroom_4k.hdr");
	//m_sceneManager->CreateCubeMap("Sky", L"D:\\blue_grotto_4k.hdr");
	//m_sceneManager->CreateCubeMap("Sky", L"D:\\HDRGPUTest.hdr");


	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。
	UpdateTransform();
	InitBoundingStructures();

	// Init Lighting
	{
		NXPBRPointLight* pPointLight;
		pPointLight = m_sceneManager->CreatePBRPointLight(Vector3(0.0f, 4.5f, 0.0f), Vector3(1.0f), 1.0f, 100.0f);
		m_cbDataLights.pointLight[0] = pPointLight->GetConstantBuffer();

		NXPBRDistantLight* pDirLight;
		pDirLight = m_sceneManager->CreatePBRDistantLight(Vector3(-1.0f, 0.0f, 1.0f), Vector3(1.0f), 2.0f);
		m_cbDataLights.distantLight[0] = pDirLight->GetConstantBuffer();

		NXPBRSpotLight* pSpotLight;
		pSpotLight = m_sceneManager->CreatePBRSpotLight(Vector3(0.0f, 2.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f), 1.0f, 30.0f, 50.0f, 100.0f);
		m_cbDataLights.spotLight[0] = pSpotLight->GetConstantBuffer();

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

		g_pContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &m_cbDataLights, 0, 0);
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
	auto pMainCamera = GetMainCamera();

	NSFirstPersonalCamera* pScript = dynamic_cast<NSFirstPersonalCamera*>(m_sceneManager->CreateScript(NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA, pMainCamera));
	
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	NXEventKeyUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	NXEventMouseMove::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseDown, pScript, std::placeholders::_1));
	NXEventMouseUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseUp, pScript, std::placeholders::_1));

	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NXScene::OnKeyDown, this, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(std::bind(&NXScene::OnMouseDown, this, std::placeholders::_1));
}

void NXScene::UpdateTransform(NXObject* pObject)
{
	// 更新pObject下所有Transform的值和脚本。
	// pObject为空时=更新场景中全部物体。
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

void NXScene::UpdateLightData()
{
	NXPBRDistantLight* pDirLight = nullptr;
	NXPBRPointLight* pPointLight = nullptr;
	NXPBRSpotLight* pSpotLight = nullptr;

	UINT dirIdx = 0;
	UINT pointIdx = 0;
	UINT spotIdx = 0;

	for (auto pLight : GetPBRLights())
	{
		switch (pLight->GetType())
		{
		case NXLight_Distant:
			pDirLight = (NXPBRDistantLight*)pLight;
			m_cbDataLights.distantLight[dirIdx++] = pDirLight->GetConstantBuffer();
			break;
		case NXLight_Point:
			pPointLight = (NXPBRPointLight*)pLight;
			m_cbDataLights.pointLight[pointIdx++] = pPointLight->GetConstantBuffer();
			break;
		case NXLight_Spot:
			pSpotLight = (NXPBRSpotLight*)pLight;
			m_cbDataLights.spotLight[spotIdx++] = pSpotLight->GetConstantBuffer();
			break;
		default:
			break;
		}
	}

	g_pContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &m_cbDataLights, 0, 0);
}

void NXScene::Release()
{
	SafeRelease(m_sceneManager);
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
			float aabbDist, aabbOutDist;
			if (ray.IntersectsFast(prim->GetAABBWorld(), aabbOutDist))
			{
				aabbDist = aabbOutDist;
				if (aabbDist < outDist)
				{
					// ray-triangle
					if (prim->RayCast(ray, outHitInfo, outDist))
					{
						// 得到了更近的相交结果。
						// 保留当前outHitInfo和outDist。
					}
				}
			}
		}
	}
	
	if (!outHitInfo.pSubMesh)
		return false;

	outHitInfo.LocalToWorld();
	return true;
}

void NXScene::InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb)
{
	//auto lights = GetPBRLights();
	//if (lights.empty())
	//	return;

	//NXPBRDistantLight* pDistantLight = nullptr;
	//for (auto pLight : lights)
	//{
	//	pDistantLight = (NXPBRDistantLight*)(pLight);
	//	if (pDistantLight) break;  // 目前仅对第一个平行光提供支持
	//}

	//Vector3 shadowMapAt = m_boundingSphere.Center;
	//Vector3 shadowMapEye = shadowMapAt - 2.0f * m_boundingSphere.Radius * pDistantLight->m_direction;
	//Vector3 shadowMapUp(0.0f, 1.0f, 0.0f);
	//Matrix mxV = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

	//Vector3 shadowMapAtInViewSpace = Vector3::Transform(shadowMapAt, mxV);
	//Vector3 OrthoBoxRangeMin = shadowMapAtInViewSpace - Vector3(m_boundingSphere.Radius);
	//Vector3 OrthoBoxRangeMax = shadowMapAtInViewSpace + Vector3(m_boundingSphere.Radius);
	//Matrix mxP = XMMatrixOrthographicOffCenterLH(OrthoBoxRangeMin.x, OrthoBoxRangeMax.x, OrthoBoxRangeMin.y, OrthoBoxRangeMax.y, OrthoBoxRangeMin.z, OrthoBoxRangeMax.z);

	//Matrix mxT(
	//	0.5f, 0.0f, 0.0f, 0.0f,
	//	0.0f, -0.5f, 0.0f, 0.0f,
	//	0.0f, 0.0f, 1.0f, 0.0f,
	//	0.5f, 0.5f, 0.0f, 1.0f);

	//out_cb.view = mxV.Transpose();
	//out_cb.projection = mxP.Transpose();
	//out_cb.texture = mxT.Transpose();
}

void NXScene::InitBoundingStructures()
{
	// construct AABB for scene.
	for (auto prim : GetPrimitives())
	{
		AABB::CreateMerged(m_aabb, m_aabb, prim->GetAABBWorld());
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

void NXScene::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	m_sceneManager->BuildBVHTrees(SplitMode);
}
