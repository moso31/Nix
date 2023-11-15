#include "NXScene.h"
#include "Global.h"
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
#include "NXGUICommandManager.h"
#include "NXGUIInspector.h"

#include "NXScript.h"
#include "NXScriptType.h"
#include "NSFirstPersonalCamera.h"
#include "NSTest.h"

// temp include.

NXScene::NXScene() :
	m_bMultiSelectKeyHolding(false),
	m_pRootObject(new NXObject()),
	m_pBVHTree(nullptr),
	m_pCubeMap(nullptr),
	m_pMainCamera(nullptr),
	m_bEditorSelectID(EditorObjectID::NONE)
{
}

NXScene::~NXScene()
{
}

void NXScene::OnMouseDown(NXEventArgMouse eArg)
{
	if (eArg.VMouse & 1) // 鼠标左键
	{
		auto ray = GetMainCamera()->GenerateRay(eArg.ViewPortPos, eArg.ViewPortSize);
		
		// 检测是否点击编辑器对象（移动箭头，旋转环，缩放轴）
		// （不过目前实际上只有移动箭头hhhhh）
		NXHit editObjHit;
		m_pEditorObjManager->RayCast(ray, editObjHit);
		if (editObjHit.pSubMesh)
		{
			NXSubMeshEditorObjects* pHitSubmesh = (NXSubMeshEditorObjects*)editObjHit.pSubMesh;
			m_bEditorSelectID = pHitSubmesh->GetEditorObjectID();

			Vector3 anchorPos = GetAnchorOfEditorObject(ray);
			for (int i = 0; i < m_pSelectedObjects.size(); i++)
			{
				auto pSelectObjs = m_pSelectedObjects[i];
				m_selectObjHitOffset[i] = anchorPos - pSelectObjs->GetWorldTranslation();
			}
		}
		else
		{
			// 按住LCtrl时多选
			if (!m_bMultiSelectKeyHolding)
			{
				m_pSelectedSubMeshes.clear();
				m_pSelectedObjects.clear();

				m_selectObjHitOffset.clear();
			}

			NXHit hit;
			RayCast(ray, hit);
			if (hit.pSubMesh)
			{
				AddPickingSubMesh(hit.pSubMesh);
				{
					NXGUICommand e(NXGUICmd_Inspector_SetIdx, { NXGUIInspector_Material });
					NXGUICommandManager::GetInstance()->PushCommand(e);
				}
			}

			// 若没有picking对象，隐藏EditorObjects
			m_pEditorObjManager->SetVisible(!m_pSelectedObjects.empty());
		}
	}
}

void NXScene::OnMouseMove(NXEventArgMouse eArg)
{
	auto worldRay = GetMainCamera()->GenerateRay(eArg.ViewPortPos, eArg.ViewPortSize);

	if (m_bEditorSelectID > EditorObjectID::NONE &&
		m_bEditorSelectID < EditorObjectID::MAX)
	{
		// 进入这里说明正在拖动 MoveArrow/RotateRing/ScaleBoxes
		
		// 获取拖动后位置在MoveArrow的轴/平面上的投影坐标
		Vector3 anchorPos = GetAnchorOfEditorObject(worldRay);

		// 移动所有选中的物体
		for (int i = 0; i < m_pSelectedObjects.size(); i++)
		{
			auto pSelectObjs = m_pSelectedObjects[i];
			pSelectObjs->SetWorldTranslation(anchorPos - m_selectObjHitOffset[i]);
		}

		if (!m_pSelectedObjects.empty())
		{
			// MoveArrow 也跟着移动位置
			// 2023.3.11 暂时使用第一个选中的Primitive的Translation计算移动量
			NXPrimitive* pFirstSelectedPrimitive = m_pSelectedObjects[0];
			m_pEditorObjManager->MoveTranslatorTo(pFirstSelectedPrimitive->GetAABBWorld().Center);
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
	if (eArg.VKey == NXKeyCode::LeftControl)
	{
		m_bMultiSelectKeyHolding = true;
	}
}

void NXScene::OnKeyUp(NXEventArgKey eArg)
{
}

void NXScene::OnKeyUpForce(NXEventArgKey eArg)
{
	if (eArg.VKey == NXKeyCode::LeftControl)
	{
		m_bMultiSelectKeyHolding = false;
	}
}

Vector3 NXScene::GetAnchorOfEditorObject(const Ray& worldRay)
{
	if (m_pSelectedObjects.empty())
		return Vector3(0.0f);

	auto pSelectObjs = m_pEditorObjManager->GetMoveArrow();

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

	m_pTestCustomMat = NXResourceManager::GetInstance()->GetMaterialManager()->CreateCustomMaterial("TestCustomMat", "D:\\NixAssets\\Materials\\mat.nsl");

	//NXPrefab* p = NXResourceManager::GetInstance()->GetMeshManager()->CreateFBXPrefab("arnia", "D:\\NixAssets\\boxes.fbx", false);
	//NXPrefab* p = NXResourceManager::GetInstance()->GetMeshManager()->CreateFBXPrefab("arnia", "D:\\NixAssets\\shadowMapTest.fbx", false);
	//NXPrefab* p = NXResourceManager::GetInstance()->GetMeshManager()->CreateFBXPrefab("arnia", "D:\\NixAssets\\EditorObjTest.fbx", false);
	NXPrefab* p = NXResourceManager::GetInstance()->GetMeshManager()->CreateFBXPrefab("arnia", "D:\\NixAssets\\lury.fbx", false);
	//NXPrefab* p = NXResourceManager::GetInstance()->GetMeshManager()->CreateFBXPrefab("arnia", "D:\\NixAssets\\testScene.fbx", false);
	p->SetScale(Vector3(0.1f));
	NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(p, m_pTestCustomMat);

	std::vector<NXPrimitive*> pMeshes;
	{
		//bool bBind = NXResourceManager::GetInstance()->BindParent(pMeshes[1], pSphere);
		//auto pScript_test = new NSTest();
		//pMeshes[0]->AddScript(pScript_test);
	}

	auto pCamera = NXResourceManager::GetInstance()->GetCameraManager()->CreateCamera(
		"Camera1",
		70.0f, 0.3f, 1000.f,
		Vector3(0.0f, 0.0f, -10.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f),
		m_rtSize
	);

	NXCubeMap* pSky =
	NXResourceManager::GetInstance()->GetLightManager()->CreateCubeMap("Sky", L"D:\\NixAssets\\HDR\\ballroom_4k.hdr");
	pSky->SetIntensity(1.0f);

	InitBoundingStructures();

	// Init Lighting
	{
		NXPBRPointLight* pPointLight;
		pPointLight = NXResourceManager::GetInstance()->GetLightManager()->CreatePBRPointLight(Vector3(0.0f, 0.25f, 0.0f), Vector3(1.0f), 100.0f, 100.0f);
		m_cbDataLights.pointLight[0] = pPointLight->GetConstantBuffer();

		NXPBRDistantLight* pDirLight;
		pDirLight = NXResourceManager::GetInstance()->GetLightManager()->CreatePBRDistantLight(Vector3(-1.0f, -1.30f, 1.0f), Vector3(1.0f), 0.0f);
		m_cbDataLights.distantLight[0] = pDirLight->GetConstantBuffer();

		NXPBRSpotLight* pSpotLight;
		//pSpotLight = NXResourceManager::GetInstance()->GetLightManager()->CreatePBRSpotLight(Vector3(0.0f, 2.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f), 1.0f, 30.0f, 50.0f, 100.0f);
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

void NXScene::OnResize(const Vector2& rtSize)
{
	m_rtSize = rtSize;

	auto pCamera = GetMainCamera();
	if (pCamera) pCamera->OnResize(rtSize);
}

void NXScene::InitScripts()
{
	auto pMainCamera = GetMainCamera();

	NSFirstPersonalCamera* pScript = dynamic_cast<NSFirstPersonalCamera*>(NXResourceManager::GetInstance()->GetScriptManager()->CreateScript(NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA, pMainCamera));
	pMainCamera->SetFirstPersonalController(pScript);

	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyDown, pScript, std::placeholders::_1));
	NXEventKeyUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnKeyUp, pScript, std::placeholders::_1));
	NXEventMouseMove::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseMove, pScript, std::placeholders::_1));
	NXEventMouseDownViewport::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseDown, pScript, std::placeholders::_1));
	NXEventMouseUp::GetInstance()->AddListener(std::bind(&NSFirstPersonalCamera::OnMouseUp, pScript, std::placeholders::_1));

	NXEventKeyDown::GetInstance()->AddListener(std::bind(&NXScene::OnKeyDown, this, std::placeholders::_1));
	NXEventKeyUp::GetInstance()->AddListener(std::bind(&NXScene::OnKeyUp, this, std::placeholders::_1));
	NXEventKeyUpForce::GetInstance()->AddListener(std::bind(&NXScene::OnKeyUpForce, this, std::placeholders::_1));
	NXEventMouseMoveViewport::GetInstance()->AddListener(std::bind(&NXScene::OnMouseMove, this, std::placeholders::_1));
	NXEventMouseDownViewport::GetInstance()->AddListener(std::bind(&NXScene::OnMouseDown, this, std::placeholders::_1));
	NXEventMouseUpViewport::GetInstance()->AddListener(std::bind(&NXScene::OnMouseUp, this, std::placeholders::_1));
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
	for (auto& obj : m_scriptableObjects)
	{
		for (auto& scr : obj->GetScripts())
		{
			scr->Update();
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
	SafeRelease(m_pBVHTree);
	SafeRelease(m_pRootObject);
}

void NXScene::AddPickingSubMesh(NXSubMeshBase* pPickingSubMesh)
{
	NXPrimitive* pPickingObj = pPickingSubMesh->GetPrimitive();
	if (m_pSelectedObjects.empty() || std::find(m_pSelectedObjects.begin(), m_pSelectedObjects.end(), pPickingObj) == m_pSelectedObjects.end())
	{
		m_pSelectedObjects.push_back(pPickingObj);
		m_selectObjHitOffset.push_back(Vector3(0.0f));
	}
		
	if (m_pSelectedSubMeshes.empty() || std::find(m_pSelectedSubMeshes.begin(), m_pSelectedSubMeshes.end(), pPickingSubMesh) == m_pSelectedSubMeshes.end())
	{
		m_pSelectedSubMeshes.push_back(pPickingSubMesh);
	}

	// 2023.3.11 暂时使用第一个选中的Primitive的Translation计算移动量
	NXPrimitive* pFirstSelectedPrimitive = m_pSelectedSubMeshes[0]->GetPrimitive();
	m_pEditorObjManager->MoveTranslatorTo(pFirstSelectedPrimitive->GetAABBWorld().Center);
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
	SafeRelease(m_pBVHTree);

	m_pBVHTree = new HBVHTree(this);
	m_pBVHTree->BuildTreesWithScene(SplitMode);
}

void NXScene::RegisterCubeMap(NXCubeMap* newCubeMap)
{
	if (m_pCubeMap)
	{
		printf("Warning: cubemap has been set already! strightly cover cubemap maybe will make some problem.\n");
	}
	m_pCubeMap = newCubeMap;
	m_scriptableObjects.push_back(newCubeMap);
	m_objects.push_back(newCubeMap);
	newCubeMap->SetParent(m_pRootObject);
}

void NXScene::RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent)
{
	m_scriptableObjects.push_back(newPrimitive);
	m_renderableObjects.push_back(newPrimitive);
	m_objects.push_back(newPrimitive);

	newPrimitive->SetParent(pParent ? pParent : m_pRootObject);
}

void NXScene::RegisterPrefab(NXPrefab* newPrefab, NXObject* pParent)
{
	m_scriptableObjects.push_back(newPrefab);
	m_renderableObjects.push_back(newPrefab);
	m_objects.push_back(newPrefab);

	newPrefab->SetParent(pParent ? pParent : m_pRootObject);
}

void NXScene::RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent)
{
	if (isMainCamera) m_pMainCamera = newCamera;
	m_scriptableObjects.push_back(newCamera);
	m_objects.push_back(newCamera);
	newCamera->SetParent(pParent ? pParent : m_pRootObject);
}

void NXScene::RegisterLight(NXPBRLight* newLight, NXObject* pParent)
{
	m_pbrLights.push_back(newLight);
}


void NXScene::InitEditorObjectsManager()
{
	m_pEditorObjManager = new NXEditorObjectManager(this);
	m_pEditorObjManager->Init();
}
