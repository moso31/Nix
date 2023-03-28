#include "SceneManager.h"
#include "NXConverter.h"

#include "NXScene.h"
#include "NXPrefab.h"
#include "NXPrimitive.h"
#include "FBXMeshLoader.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXResourceReloader.h"

#include "NSFirstPersonalCamera.h"

NXScene* SceneManager::s_pWorkingScene = nullptr;

void SceneManager::SetWorkingScene(NXScene* pScene)
{
	s_pWorkingScene = pScene;
}

void SceneManager::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	SafeRelease(s_pWorkingScene->m_pBVHTree);

	s_pWorkingScene->m_pBVHTree = new HBVHTree(s_pWorkingScene);
	s_pWorkingScene->m_pBVHTree->BuildTreesWithScene(SplitMode);
}

NXScript* SceneManager::CreateScript(const NXScriptType scriptType, NXObject* pObject)
{
	switch (scriptType)
	{
	case NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA:
	{
		auto pScript = new NSFirstPersonalCamera();
		pObject->AddScript(pScript);
		//m_scripts.push_back(pScript);
		return pScript;
	}
	default:
		return nullptr;
	}
}

NXPrimitive* SceneManager::CreateBox(const std::string name, const float width, const float height, const float length, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateBox(p, width, height, length);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPrimitive* SceneManager::CreateSphere(const std::string name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateSphere(p, radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPrimitive* SceneManager::CreateSHSphere(const std::string name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateSHSphere(p, l, m, radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPrimitive* SceneManager::CreateCylinder(const std::string name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateCylinder(p, radius, length, segmentCircle, segmentLength);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPrimitive* SceneManager::CreatePlane(const std::string name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreatePlane(p, width, height, axis);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPrefab* SceneManager::CreateFBXPrefab(const std::string name, const std::string filePath, bool bAutoCalcTangents)
{
	auto p = new NXPrefab();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateFBXPrefab(p, filePath, bAutoCalcTangents);
	RegisterPrefab(p);
	return p;
}

NXCamera* SceneManager::CreateCamera(const std::string name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up)
{
	auto p = new NXCamera();
	p->Init(FovY, zNear, zFar, eye, at, up);

	RegisterCamera(p, true);
	return p;
}

void SceneManager::BindMaterial(NXRenderableObject* pRenderableObj, NXMaterial* pMaterial)
{
	NXPrimitive* pPrimitive = pRenderableObj->IsPrimitive();
	if (pPrimitive)
	{
		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubMesh = pPrimitive->GetSubMesh(i);
			BindMaterial(pSubMesh, pMaterial);
		}
	}

	for (auto pChild : pRenderableObj->GetChilds())
	{
		if (pChild->IsRenderableObject())
		{
			NXRenderableObject* pChildObj = static_cast<NXRenderableObject*>(pChild);
			BindMaterial(pChildObj, pMaterial);
		}
	}
}

void SceneManager::BindMaterial(NXSubMeshBase* pSubMesh, NXMaterial* pMaterial)
{
	auto pOldMat = pSubMesh->GetMaterial();
	if (pOldMat)
		pOldMat->RemoveSubMesh(pSubMesh);

	pSubMesh->SetMaterial(pMaterial);
	pMaterial->AddSubMesh(pSubMesh);
}

NXPBRDistantLight* SceneManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance)
{
	auto pLight = new NXPBRDistantLight(direction, color, illuminance);
	RegisterLight(pLight);
	return pLight;
}

NXPBRPointLight* SceneManager::CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius)
{
	auto pLight = new NXPBRPointLight(position, color, intensity, influenceRadius);
	RegisterLight(pLight);
	return pLight;
}

NXPBRSpotLight* SceneManager::CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius)
{
	auto pLight = new NXPBRSpotLight(position, direction, color, intensity, innerAngle, outerAngle, influenceRadius);
	RegisterLight(pLight);
	return pLight;
}

NXCubeMap* SceneManager::CreateCubeMap(const std::string name, const std::filesystem::path& filePath)
{
	auto pCubeMap = new NXCubeMap(s_pWorkingScene);
	pCubeMap->SetName(name);
	pCubeMap->Init(filePath);
	RegisterCubeMap(pCubeMap);
	return pCubeMap;
}

bool SceneManager::BindParent(NXObject* pParent, NXObject* pChild)
{
	if (!pChild || !pParent)
		return false;

	pChild->SetParent(pParent);
	return true;
}

void SceneManager::Release()
{
}

void SceneManager::RegisterCubeMap(NXCubeMap* newCubeMap)
{
	if (s_pWorkingScene->m_pCubeMap)
	{
		printf("Warning: cubemap has been set already! strightly cover cubemap maybe will make some problem.\n");
	}
	s_pWorkingScene->m_pCubeMap = newCubeMap;
	s_pWorkingScene->m_objects.push_back(newCubeMap);
	newCubeMap->SetParent(s_pWorkingScene->m_pRootObject);
}

void SceneManager::RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent)
{
	s_pWorkingScene->m_renderableObjects.push_back(newPrimitive);
	s_pWorkingScene->m_objects.push_back(newPrimitive);

	newPrimitive->SetParent(pParent ? pParent : s_pWorkingScene->m_pRootObject);
}

void SceneManager::RegisterPrefab(NXPrefab* newPrefab, NXObject* pParent)
{
	s_pWorkingScene->m_renderableObjects.push_back(newPrefab);
	s_pWorkingScene->m_objects.push_back(newPrefab);

	newPrefab->SetParent(pParent ? pParent : s_pWorkingScene->m_pRootObject);
}

void SceneManager::RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent)
{
	if (isMainCamera) s_pWorkingScene->m_pMainCamera = newCamera;
	s_pWorkingScene->m_objects.push_back(newCamera);
	newCamera->SetParent(pParent ? pParent : s_pWorkingScene->m_pRootObject);
}

void SceneManager::RegisterLight(NXPBRLight* newLight, NXObject* pParent)
{
	s_pWorkingScene->m_pbrLights.push_back(newLight);
}
