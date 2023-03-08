#include "NXScene.h"
#include "SceneManager.h"
#include "NXEditorObjectManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "GlobalBufferManager.h"
#include "NXIntersection.h"
#include "NXRandom.h"

#include "NXPrefab.h"
#include "NXPrimitive.h"
#include "NXCamera.h"

#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

#include "NXCubeMap.h"

#include "NXScript.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

NXScene::NXScene() :
	m_pPickingObject(nullptr),
	m_pRootObject(new NXObject()),
	m_pBVHTree(nullptr),
	m_pCubeMap(nullptr),
	m_pMainCamera(nullptr),
	m_bEditorSelectID(EditorObjectID::NONE),
	m_editorHitOffset(0.0f)
{
	m_type = NXType::eScene;
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArgMouse eArg)
{
	if (eArg.VMouse & 1) // 鼠标左键
	{
		auto ray = GetMainCamera()->GenerateRay(Vector2(eArg.X + 0.5f, eArg.Y + 0.5f));
		//printf("cursor: %.3f, %.3f\n", (float)eArg.X, (float)eArg.Y);
		//printf("pos: %.3f, %.3f, %.3f; dir: %.3f, %.3f, %.3f\n", ray.position.x, ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
		
		NXHit editObjHit;
		m_pEditorObjManager->RayCast(ray, editObjHit);
		if (editObjHit.pSubMesh)
		{
			NXSubMeshEditorObjects* pHitSubmesh = (NXSubMeshEditorObjects*)editObjHit.pSubMesh;
			m_bEditorSelectID = pHitSubmesh->GetEditorObjectID();

			if (!m_selectedObjects.empty())
			{
				Vector3 anchorPos = GetAnchorOfEditorObject(ray);
				m_editorHitOffset = anchorPos - m_selectedObjects[0]->GetWorldTranslation();
			}
		}
		else
		{
			m_selectedObjects.clear();

			NXHit hit;
			RayCast(ray, hit);
			if (hit.pSubMesh)
			{
				SetCurrentPickingSubMesh(hit.pSubMesh);
			}
		}
	}
}

void NXScene::OnMouseMove(NXEventArgMouse eArg)
{
	auto worldRay = GetMainCamera()->GenerateRay(Vector2(eArg.X + 0.5f, eArg.Y + 0.5f));

	if (m_bEditorSelectID > EditorObjectID::NONE &&
		m_bEditorSelectID < EditorObjectID::MAX)
	{
		// 进入这里说明正在拖动 SelectionArrow/RotateRing/ScaleBoxes

		Vector3 anchorPos = GetAnchorOfEditorObject(worldRay);

		// 【2022.9.28 目前只支持单选。将来改】
		if (!m_selectedObjects.empty())
		{
			auto pSelectObjs = m_selectedObjects[0];

			Vector3 targetPosWS = anchorPos - m_editorHitOffset;
			pSelectObjs->SetWorldTranslation(targetPosWS);
			m_pEditorObjManager->MoveTranslatorTo(pSelectObjs->GetAABBWorld().Center);
		}
	}

	NXHit editObjHit;
	m_pEditorObjManager->SetHighLightID(EditorObjectID::NONE);
	m_pEditorObjManager->RayCast(worldRay, editObjHit);
	if (editObjHit.pSubMesh)
	{
		// 鼠标移动到了EditorObject上方（触发EditorObject高亮逻辑）
		NXSubMeshEditorObjects* pHitSubmesh = (NXSubMeshEditorObjects*)editObjHit.pSubMesh;
		m_pEditorObjManager->SetHighLightID(pHitSubmesh->GetEditorObjectID());
	}
}

void NXScene::OnMouseUp(NXEventArgMouse eArg)
{
	if (eArg.VMouse & 2) // 鼠标左键
	{
		m_bEditorSelectID = EditorObjectID::NONE;
	}
}

void NXScene::OnKeyDown(NXEventArgKey eArg)
{
}

Vector3 NXScene::GetAnchorOfEditorObject(const Ray& worldRay)
{
	if (m_selectedObjects.empty()) 
		return Vector3(0.0f);

	// 【2022.9.28 目前只支持单选。将来改】
	auto pSelectObjs = m_selectedObjects[0];

	if (m_bEditorSelectID >= EditorObjectID::TRANSLATE_X &&
		m_bEditorSelectID <= EditorObjectID::TRANSLATE_Z)
	{
		Vector3 worldAxis = m_bEditorSelectID == EditorObjectID::TRANSLATE_X ? Vector3(1.0f, 0.0f, 0.0f) :
			m_bEditorSelectID == EditorObjectID::TRANSLATE_Y ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(0.0f, 0.0f, 1.0f);
		Ray line(pSelectObjs->GetWorldTranslation(), worldAxis);
		return GetAnchorOfEditorTranslatorLine(worldRay, line);
	}

	if (m_bEditorSelectID >= EditorObjectID::TRANSLATE_XY &&
		m_bEditorSelectID <= EditorObjectID::TRANSLATE_YZ)
	{
		Vector3 worldNormal = m_bEditorSelectID == EditorObjectID::TRANSLATE_YZ ? Vector3(1.0f, 0.0f, 0.0f) :
			m_bEditorSelectID == EditorObjectID::TRANSLATE_XZ ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(0.0f, 0.0f, 1.0f);
		Plane plane(pSelectObjs->GetWorldTranslation(), worldNormal);
		return GetAnchorOfEditorTranslatorPlane(worldRay, plane);
	}

	return Vector3(0.0f);
}

Vector3 NXScene::GetAnchorOfEditorTranslatorLine(const Ray& ray, const Ray& line) const
{
	// 求当前直线line离射线的最近点。
	// 思路是让射线和直线叉积，然后利用 射线+叉积向量=直线上最近点的思路，列出三个向量（射线、叉积向量、直线），再高斯消元求解。
	Vector3 p1 = ray.position;
	Vector3 d1 = ray.direction;
	Vector3 p2 = line.position;
	Vector3 d2 = line.direction;
	Vector3 d3 = d1.Cross(d2);
	Vector3 v = p2 - p1;

	// 此时有
	// [ d1.x -d2.x d3.x ] [ a ] [ p2.x - p1.x ]
	// [ d1.y -d2.y d3.y ] [ b ] [ p2.y - p1.y ]
	// [ d1.z -d2.z d3.z ] [ c ] [ p2.z - p1.z ]
	// 可记作增广矩阵 (这里把 b行 移到了下面)
	// [ d1.x d3.x -d2.x | v.x ]
	// [ d1.y d3.y -d2.y | v.y ]
	// [ d1.z d3.z -d2.z | v.z ]
	Vector4 f1(d1.x, d3.x, -d2.x, v.x);
	Vector4 f2(d1.y, d3.y, -d2.y, v.y);
	Vector4 f3(d1.z, d3.z, -d2.z, v.z);
	// 然后高斯消元把上面这个矩阵撅了就行（悲）
	// 第一轮消元
	float invf1x = 1.0f / f1.x;
	f2 -= f1 * (f2.x * invf1x);
	f3 -= f1 * (f3.x * invf1x);
	// 第二轮消元
	f3 -= f2 * (f3.y / f2.y);

	// 于是 f3.w / f3.z = b
	// p2 + b * d2 即为直线(p2d2)离射线的最近点。
	Vector3 nearestPoint = p2 + d2 * (f3.w / f3.z);
	return nearestPoint;
}

Vector3 NXScene::GetAnchorOfEditorTranslatorPlane(const Ray& ray, const Plane& plane) const
{
	float dist;
	if (ray.Intersects(plane, dist))
		return ray.position + ray.direction * dist;
	return Vector3();
}

void NXScene::Init()
{
	InitEditorObjectsManager();

	NXPBRMaterialStandard* pPBRMat[] = {
		SceneManager::GetInstance()->CreatePBRMaterialStandard("rustediron2", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		SceneManager::GetInstance()->CreatePBRMaterialStandard("hex-stones1", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		SceneManager::GetInstance()->CreatePBRMaterialStandard("pirate-gold", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
		SceneManager::GetInstance()->CreatePBRMaterialStandard("circle-textured-metal1", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f),
	};

	pPBRMat[0]->SetTexAlbedo(L"D:\\NixAssets\\rustediron2\\albedo.png", true);
	pPBRMat[0]->SetTexNormal(L"D:\\NixAssets\\rustediron2\\normal.png", true);
	pPBRMat[0]->SetTexMetallic(L"D:\\NixAssets\\rustediron2\\metallic.png", true);
	pPBRMat[0]->SetTexRoughness(L"D:\\NixAssets\\rustediron2\\roughness.png", true);
	pPBRMat[0]->SetTexAO(L"D:\\NixAssets\\rustediron2\\ao.png", true);

	//pPBRMat[1]->SetTexAlbedo(L"D:\\NixAssets\\hex-stones1\\albedo.png", true);
	//pPBRMat[1]->SetTexNormal(L"D:\\NixAssets\\hex-stones1\\normal.png", true);
	//pPBRMat[1]->SetTexMetallic(L"D:\\NixAssets\\hex-stones1\\metallic.png", true);
	//pPBRMat[1]->SetTexRoughness(L"D:\\NixAssets\\hex-stones1\\roughness.png", true);
	//pPBRMat[1]->SetTexAO(L"D:\\NixAssets\\hex-stones1\\ao.png", true);

	//pPBRMat[2]->SetTexAlbedo(L"D:\\NixAssets\\pirate-gold\\albedo.png", true);
	//pPBRMat[2]->SetTexNormal(L"D:\\NixAssets\\pirate-gold\\normal.png", true);
	//pPBRMat[2]->SetTexMetallic(L"D:\\NixAssets\\pirate-gold\\metallic.png", true);
	//pPBRMat[2]->SetTexRoughness(L"D:\\NixAssets\\pirate-gold\\roughness.png", true);
	//pPBRMat[2]->SetTexAO(L"D:\\NixAssets\\pirate-gold\\ao.png", true);

	//pPBRMat[3]->SetTexAlbedo(L"D:\\NixAssets\\circle-textured-metal1\\albedo.png", true);
	//pPBRMat[3]->SetTexNormal(L"D:\\NixAssets\\circle-textured-metal1\\normal.png", true);
	//pPBRMat[3]->SetTexMetallic(L"D:\\NixAssets\\circle-textured-metal1\\metallic.png", true);
	//pPBRMat[3]->SetTexRoughness(L"D:\\NixAssets\\circle-textured-metal1\\roughness.png", true);
	//pPBRMat[3]->SetTexAO(L"D:\\NixAssets\\circle-textured-metal1\\ao.png", true);

	//auto pSphere = SceneManager::GetInstance()->CreateSphere("Sphere", 1.0f, 64, 64);
	//SceneManager::GetInstance()->BindMaterial(pSphere->GetSubMesh(0), pPBRMat[0]);

	//for (int l = 0; l < 4; l++)
	//{
	//	for (int m = -l; m <= l; m++)
	//	{
	//		Vector3 objPos(m * 1.5f, -l * 1.5f, 0.0f);
	//		auto pSH = SceneManager::GetInstance()->CreateSHSphere("shTest", l, m, 1.0f, 64, 64, objPos);
	//		SceneManager::GetInstance()->BindMaterial(pSH->GetSubMesh(0), pPBRMat[0]);
	//		pSH->AddScript(new NSTest());
	//	}
	//}

	//auto p = SceneManager::GetInstance()->CreatePlane("Sphere", 10.0f, 10.0f, NXPlaneAxis::POSITIVE_Y);
	//SceneManager::GetInstance()->BindMaterial(p->GetSubMesh(0), pPBRMat[0]);

	std::vector<NXPrimitive*> pMeshes;
	//SceneManager::GetInstance()->CreateFBXMeshes("D:\\NixAssets\\UnityBall.fbx", pMeshes);
	//SceneManager::GetInstance()->BindMaterial(pMeshes[0]->GetSubMesh(0), pPBRMat[0]);
	//SceneManager::GetInstance()->BindMaterial(pMeshes[0]->GetSubMesh(1), pPBRMat[1]);
	//SceneManager::GetInstance()->BindMaterial(pMeshes[0]->GetSubMesh(2), pPBRMat[3]);
	//SceneManager::GetInstance()->BindMaterial(pMeshes[0]->GetSubMesh(3), pPBRMat[3]);
	//pMeshes[0]->SetRotation(Vector3(-0.8f, 0.0f, 0.0f));

	//for (int i = -5; i <= 5; i++)
	//{
	//	auto pPBRMat = SceneManager::GetInstance()->CreatePBRMaterialTranslucent("rustediron2", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f, 0.2f);
	//	auto pSphere = SceneManager::GetInstance()->CreateSphere("Sphere", 1.0f, 64, 64, Vector3(i * 2.0f, 0.0f, 0.0f));
	//	SceneManager::GetInstance()->BindMaterial(pSphere->GetSubMesh(0), pPBRMat);

	//	auto pPBRMatS = SceneManager::GetInstance()->CreatePBRMaterialStandard("rustediron2", Vector3(1.0f), Vector3(1.0f), 1.0f, 0.0f, 1.0f);
	//	pSphere = SceneManager::GetInstance()->CreateSphere("Sphere", 1.0f, 64, 64, Vector3(i * 2.0f, 2.0f, 0.0f));
	//	SceneManager::GetInstance()->BindMaterial(pSphere->GetSubMesh(0), pPBRMatS);
	//}

	auto* pPlane = SceneManager::GetInstance()->CreatePlane("G", 1000.0f, 1000.0f, NXPlaneAxis::POSITIVE_Y);
	SceneManager::GetInstance()->BindMaterial(pPlane, pPBRMat[0]);

	//SceneManager::GetInstance()->CreateFBXMeshes("D:\\NixAssets\\Cloth.fbx", pMeshes, true);
	//SceneManager::GetInstance()->BindMaterial(pMeshes[0]->GetSubMesh(0), pPBRMat[0]);

	//NXPrefab* p = SceneManager::GetInstance()->CreateFBXPrefab("arnia", "D:\\NixAssets\\boxes.fbx", false);
	//NXPrefab* p = SceneManager::GetInstance()->CreateFBXPrefab("arnia", "D:\\NixAssets\\shadowMapTest.fbx", false);
	//NXPrefab* p = SceneManager::GetInstance()->CreateFBXPrefab("arnia", "D:\\NixAssets\\EditorObjTest.fbx", false);
	//NXPrefab* p = SceneManager::GetInstance()->CreateFBXPrefab("arnia", "D:\\NixAssets\\lury.fbx", false);
	//NXPrefab* p = SceneManager::GetInstance()->CreateFBXPrefab("arnia", "D:\\NixAssets\\testScene.fbx", false);
	//p->SetScale(Vector3(0.1f));
	//SceneManager::GetInstance()->BindMaterial(p, pPBRMat[0]);
	{
		//bool bBind = SceneManager::GetInstance()->BindParent(pMeshes[1], pSphere);
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
	//		pSphere = SceneManager::GetInstance()->CreateSphere("Sphere", 1.0f, 64, 64, pos);
	//			SceneManager::GetInstance()->BindMaterial(pSphere->GetSubMesh(0), pPBRMat[0]);
	//	}
	//}

	auto pCamera = SceneManager::GetInstance()->CreateCamera(
		"Camera1",
		70.0f, 0.3f, 1000.f,
		Vector3(0.0f, 0.0f, -10.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	NXCubeMap* pSky =
	SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\Alexs_Apt_2k.hdr");
	//SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\TexturesCom_JapanInariTempleH_1K_hdri_sphere.hdr");
	//SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\ballroom_4k.hdr");
	//SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\blue_grotto_4k.hdr");
	//SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\HDRGPUTest.hdr");
	//SceneManager::GetInstance()->CreateCubeMap("Sky", L"D:\\WhiteHDRI.hdr");
	pSky->SetIntensity(0.0f);

	InitBoundingStructures();

	// Init Lighting
	{
		NXPBRPointLight* pPointLight;
		pPointLight = SceneManager::GetInstance()->CreatePBRPointLight(Vector3(0.0f, 0.25f, 0.0f), Vector3(1.0f), 100.0f, 100.0f);
		m_cbDataLights.pointLight[0] = pPointLight->GetConstantBuffer();

		NXPBRDistantLight* pDirLight;
		pDirLight = SceneManager::GetInstance()->CreatePBRDistantLight(Vector3(-1.0f, -1.30f, 1.0f), Vector3(1.0f), 0.0f);
		m_cbDataLights.distantLight[0] = pDirLight->GetConstantBuffer();

		NXPBRSpotLight* pSpotLight;
		//pSpotLight = SceneManager::GetInstance()->CreatePBRSpotLight(Vector3(0.0f, 2.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f), 1.0f, 30.0f, 50.0f, 100.0f);
		//m_cbDataLights.spotLight[0] = pSpotLight->GetConstantBuffer();

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(ConstantBufferLight);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbLights));

		g_pContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &m_cbDataLights, 0, 0);
	}

	InitScripts();
}

void NXScene::InitScripts()
{
	auto pMainCamera = GetMainCamera();

	NSFirstPersonalCamera* pScript = dynamic_cast<NSFirstPersonalCamera*>(SceneManager::GetInstance()->CreateScript(NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA, pMainCamera));
	pMainCamera->SetFirstPersonalController(pScript);

	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	NXEventKeyUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	NXEventMouseMove::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseDown, pScript, std::placeholders::_1));
	NXEventMouseUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseUp, pScript, std::placeholders::_1));

	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NXScene::OnKeyDown, this, std::placeholders::_1));
	NXEventMouseMove::GetInstance()->AddListener(std::bind(&NXScene::OnMouseMove, this, std::placeholders::_1));
	NXEventMouseDown::GetInstance()->AddListener(std::bind(&NXScene::OnMouseDown, this, std::placeholders::_1));
	NXEventMouseUp::GetInstance()->AddListener(std::bind(&NXScene::OnMouseUp, this, std::placeholders::_1));
}

void NXScene::UpdateTransform(NXObject* pObject)
{
	// 更新pObject下所有Transform的值和脚本。
	// pObject为空时=更新场景中全部物体。
	if (!pObject)
	{
		UpdateTransform(m_pRootObject);
	}
	else
	{
		NXTransform* pT = pObject->IsTransform();
		if (pT) pT->UpdateTransform();

		auto ch = pObject->GetChilds();
		for (auto it = ch.begin(); it != ch.end(); it++)
		{
			UpdateTransform(*it);
		}
	}
}

void NXScene::UpdateTransformOfEditorObjects()
{
	m_pEditorObjManager->UpdateTransform();
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
	SafeRelease(m_pEditorObjManager);
	for (auto pLight : m_pbrLights) SafeDelete(pLight);
	for (auto pMat : m_materials) SafeRelease(pMat);
	SafeRelease(m_pBVHTree);
	SafeRelease(m_pRootObject);
}

void NXScene::SetCurrentPickingSubMesh(NXSubMeshBase* pPickingObject)
{
	m_pPickingObject = pPickingObject;

	NXPrimitive* pPickingPrimitive = pPickingObject->GetPrimitive();
	if (pPickingPrimitive)
	{
		m_pEditorObjManager->MoveTranslatorTo(pPickingPrimitive->GetAABBWorld().Center);

		m_selectedObjects.push_back(m_pPickingObject->GetPrimitive());
	}
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
		for (auto pMesh : m_renderableObjects)
		{
			if (pMesh->RayCast(ray, outHitInfo, outDist))
			{
				// success hit.
			}
		}
	}

	if (!outHitInfo.pSubMesh)
		return false;

	outHitInfo.LocalToWorld();
	return true;
}

void NXScene::InitBoundingStructures()
{
	// 要计算所有物体的AABB，就需要确保所有物体的 Transform 是正确的
	// 所以这里需要做一次 UpdateTransform
	UpdateTransform();

	// construct AABB for scene.
	// 遍历 renderableObjects，只对一级 Hierarchy 节点 InitAABB
	// 子Object 的 AABB 会递归生成。
	for (auto pMesh : m_renderableObjects)
	{
		pMesh->InitAABB();
		AABB::CreateMerged(m_aabb, m_aabb, pMesh->GetAABBWorld());
	}

	BoundingSphere::CreateFromBoundingBox(m_boundingSphere, m_aabb);
}

void NXScene::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	SceneManager::GetInstance()->BuildBVHTrees(SplitMode);
}

void NXScene::InitEditorObjectsManager()
{
	m_pEditorObjManager = new NXEditorObjectManager(this);
	m_pEditorObjManager->Init();
}
