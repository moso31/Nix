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
	m_sceneManager(new SceneManager(this))
{
	m_type = NXType::eScene;
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArgMouse eArg)
{
	auto ray = GetMainCamera()->GenerateRay(Vector2(eArg.X, eArg.Y));
	//printf("cursor: %.3f, %.3f\n", (float)eArg.X, (float)eArg.Y);
	//printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
}

void NXScene::OnKeyDown(NXEventArgKey eArg)
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
		// 创建求交加速结构以增加渲染速度。
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
		m_sceneManager->CreatePBRMaterial(Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	};

	pPBRMat[0]->SetTexAlbedo(L"D:\\NixAssets\\rustediron2\\albedo.png");
	pPBRMat[0]->SetTexNormal(L"D:\\NixAssets\\rustediron2\\normal.png");
	pPBRMat[0]->SetTexMetallic(L"D:\\NixAssets\\rustediron2\\metallic.png");
	pPBRMat[0]->SetTexRoughness(L"D:\\NixAssets\\rustediron2\\roughness.png");
	pPBRMat[0]->SetTexAO(L"D:\\NixAssets\\rustediron2\\ao.png");

	auto pSphere = m_sceneManager->CreateSphere("Sphere", 1.0f, 64, 64); 
	pSphere->SetMaterialPBR(pPBRMat[0]);

	{
		//bool bBind = m_sceneManager->BindParent(pMeshes[1], pSphere);
		auto pScript_test = new NSTest();
		pSphere->AddScript(pScript_test);
	}

	// 设置Picking Object（Demo用，临时）
	SetCurrentPickingObject(pSphere);

	auto pCamera = m_sceneManager->CreateCamera(
		"Camera1",
		70.0f, 0.01f, 1000.f,
		Vector3(0.0f, 0.0f, -2.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	m_sceneManager->CreateCubeMap("Sky", L"D:\\Alexs_Apt_2k.hdr");

	// 更新AABB需要世界坐标，而Init阶段还没有拿到世界坐标，所以需要提前PrevUpdate一次。
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
		g_pContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &m_cbDataLights.pointLight, 0, 0);
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

	NXEventKeyDown::GetInstance().AddListener(std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	NXEventKeyUp::GetInstance().AddListener(std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	NXEventMouseMove::GetInstance().AddListener(std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventMouseDown::GetInstance().AddListener(std::bind(&NSFirstPersonalCamera::OnMouseDown, pScript, std::placeholders::_1));
	NXEventMouseUp::GetInstance().AddListener(std::bind(&NSFirstPersonalCamera::OnMouseUp, pScript, std::placeholders::_1));

	NXEventKeyDown::GetInstance().AddListener(std::bind(&NXScene::OnKeyDown, this, std::placeholders::_1));
	NXEventMouseDown::GetInstance().AddListener(std::bind(&NXScene::OnMouseDown, this, std::placeholders::_1));
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
			float aabbDist;
			if (ray.IntersectsFast(prim->GetAABBWorld(), aabbDist))
			{
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
