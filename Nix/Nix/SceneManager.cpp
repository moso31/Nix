#include "SceneManager.h"
#include "NXScene.h"
#include "FBXMeshLoader.h"

#include "NSFirstPersonalCamera.h"

SceneManager::SceneManager()
{
}

SceneManager::SceneManager(NXScene* pScene) :
	m_pScene(pScene), 
	m_pRootObject(new NXObject()),
	m_pBVHTree(nullptr),
	m_pCubeMap(nullptr),
	m_pMainCamera(nullptr)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::BuildBVHTrees(const HBVHSplitMode SplitMode)
{
	if (m_pBVHTree)
	{
		m_pBVHTree->Release();
		delete m_pBVHTree;
	}

	m_pBVHTree = new HBVHTree(m_pScene, m_primitives);
	m_pBVHTree->BuildTreesWithScene(SplitMode);
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

NXListener* SceneManager::AddEventListener(const NXEventType eventType, NXObject* pObject, const std::function<void(NXEventArg)>& pFunc)
{
	auto pListener = new NXListener(pObject, pFunc);
	switch (eventType)
	{
	case NXEventType::NXEVENT_KEYDOWN:
	{
		NXEventKeyDown::GetInstance().AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_KEYUP:
	{
		NXEventKeyUp::GetInstance().AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEDOWN:
	{
		NXEventMouseDown::GetInstance().AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEUP:
	{
		NXEventMouseUp::GetInstance().AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEMOVE:
	{
		NXEventMouseMove::GetInstance().AddListener(pListener);
		break;
	}
	default:
		delete pListener;
		return nullptr;
	}
	return pListener;
}

NXBox* SceneManager::CreateBox(const std::string& name, const float width, const float height, const float length, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXBox();
	p->SetName(name);
	p->Init(width, height, length);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXSphere* SceneManager::CreateSphere(const std::string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXSphere();
	p->SetName(name);
	p->Init(radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXCylinder* SceneManager::CreateCylinder(const std::string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXCylinder();
	p->SetName(name);
	p->Init(radius, length, segmentCircle, segmentLength);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

NXPlane* SceneManager::CreatePlane(const std::string& name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPlane();
	p->SetName(name);
	p->Init(width, height, axis);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	RegisterPrimitive(p);
	return p;
}

bool SceneManager::CreateFBXMeshes(const std::string& filePath, NXPBRMaterial* pDefaultMaterial, std::vector<NXMesh*>& outMeshes, bool bAutoCalcTangents)
{
	FBXMeshLoader::LoadFBXFile(filePath, m_pScene, outMeshes, bAutoCalcTangents);
	for (auto it = outMeshes.begin(); it != outMeshes.end(); it++)
	{
		(*it)->SetMaterialPBR(pDefaultMaterial);
		RegisterPrimitive(*it, (*it)->GetParent());
	}
	return true;
}

NXCamera* SceneManager::CreateCamera(const std::string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up)
{
	auto p = new NXCamera();
	p->Init(FovY, zNear, zFar, eye, at, up);

	RegisterCamera(p, true);
	return p;
}

NXPBRMaterial* SceneManager::CreatePBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR)
{
	auto pMat = new NXPBRMaterial(albedo, metallic, roughness, reflectivity, refractivity, IOR);
	RegisterMaterial(pMat);
	return pMat;
}

NXPBRPointLight* SceneManager::CreatePBRPointLight(const Vector3& position, const Vector3& intensity)
{
	auto pLight = new NXPBRPointLight(position, intensity);
	RegisterLight(pLight);
	return pLight;
}

NXPBRDistantLight* SceneManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& radiance)
{
	auto bound = m_pScene->GetBoundingSphere();
	auto pLight = new NXPBRDistantLight(direction, radiance, bound.Center, bound.Radius);
	RegisterLight(pLight);
	return pLight;
}

NXPBRTangibleLight* SceneManager::CreatePBRTangibleLight(NXPrimitive* pPrimitive, const Vector3& radiance)
{
	auto pLight = new NXPBRTangibleLight(pPrimitive, radiance);
	pPrimitive->SetTangibleLight(pLight);
	RegisterLight(pLight);
	return pLight;
}

NXPBREnvironmentLight* SceneManager::CreatePBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& Intensity)
{
	auto bound = m_pScene->GetBoundingSphere();
	auto pLight = new NXPBREnvironmentLight(pCubeMap, Intensity, bound.Center, bound.Radius);
	pCubeMap->SetEnvironmentLight(pLight);
	RegisterLight(pLight);
	return pLight;
}

NXCubeMap* SceneManager::CreateCubeMap(const std::string& name, const std::wstring& filePath)
{
	auto pCubeMap = new NXCubeMap(m_pScene);
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
	for (auto pLight : m_pbrLights)
	{
		delete pLight;
	}

	for (auto pMat : m_pbrMaterials)
	{
		pMat->Release();
		delete pMat;
	}

	//for (auto script : m_scripts)
	//{
	//	delete script;
	//}

	if (m_pBVHTree)
	{
		m_pBVHTree->Release();
		delete m_pBVHTree;
	}

	for (auto obj : m_objects)
	{
		obj->Release();
		delete obj;
	}

	m_pRootObject->Release();
	delete m_pRootObject;
}

void SceneManager::RegisterCubeMap(NXCubeMap* newCubeMap)
{
	if (m_pCubeMap)
	{
		printf("Warning: cubemap has been set already! strightly cover cubemap maybe will make some problem.\n");
	}
	m_pCubeMap = newCubeMap;
	m_objects.push_back(newCubeMap);
	newCubeMap->SetParent(m_pRootObject);
}

void SceneManager::RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent)
{
	m_primitives.push_back(newPrimitive);
	m_objects.push_back(newPrimitive);

	newPrimitive->SetParent(pParent ? pParent : m_pRootObject);
}

void SceneManager::RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent)
{
	if (isMainCamera) m_pMainCamera = newCamera;
	m_objects.push_back(newCamera);
	newCamera->SetParent(pParent ? pParent : m_pRootObject);
}

void SceneManager::RegisterMaterial(NXPBRMaterial* newMaterial)
{
	m_pbrMaterials.push_back(newMaterial);
}

void SceneManager::RegisterLight(NXPBRLight* newLight, NXObject* pParent)
{
	m_pbrLights.push_back(newLight);
}
