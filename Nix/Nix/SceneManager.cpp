#include "SceneManager.h"
#include "NSFirstPersonalCamera.h"
#include "FBXMeshLoader.h"
#include "NXCubeMap.h"

SceneManager::SceneManager()
{
}

SceneManager::SceneManager(const std::shared_ptr<NXScene>& pScene) :
	m_scene(pScene)
{
}

SceneManager::~SceneManager()
{
}

std::shared_ptr<NXScript> SceneManager::CreateScript(const NXScriptType scriptType, const std::shared_ptr<NXObject>& pObject)
{
	switch (scriptType)
	{
	case NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA:
	{
		auto pScript = std::make_shared<NSFirstPersonalCamera>();
		//m_scene->GetScripts().push_back(pScript);
		return pScript;
	}
	default:
		return nullptr;
	}
}

std::shared_ptr<NXListener> SceneManager::AddEventListener(const NXEventType eventType, const std::shared_ptr<NXObject>& pObject, const std::function<void(NXEventArg)>& pFunc)
{
	auto pListener = std::make_shared<NXListener>(pObject, pFunc);
	switch (eventType)
	{
	case NXEventType::NXEVENT_KEYDOWN:
	{
		NXEventKeyDown::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_KEYUP:
	{
		NXEventKeyUp::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEDOWN:
	{
		NXEventMouseDown::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEUP:
	{
		NXEventMouseUp::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEMOVE:
	{
		NXEventMouseMove::GetInstance()->AddListener(pListener);
		break;
	}
	default:
		return nullptr;
	}
	return pListener;
}

std::shared_ptr<NXBox> SceneManager::CreateBox(const std::string& name, const float width, const float height, const float length, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = std::make_shared<NXBox>();
	p->SetName(name);
	p->Init(width, height, length);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

std::shared_ptr<NXSphere> SceneManager::CreateSphere(const std::string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = std::make_shared<NXSphere>();
	p->SetName(name);
	p->Init(radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

std::shared_ptr<NXCylinder> SceneManager::CreateCylinder(const std::string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = std::make_shared<NXCylinder>();
	p->SetName(name);
	p->Init(radius, length, segmentCircle, segmentLength);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

std::shared_ptr<NXPlane> SceneManager::CreatePlane(const std::string& name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = std::make_shared<NXPlane>();
	p->SetName(name);
	p->Init(width, height, axis);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

bool SceneManager::CreateFBXMeshes(const std::string& filePath, const std::shared_ptr<NXPBRMaterial>& pDefaultMaterial, std::vector<std::shared_ptr<NXMesh>>& outMeshes)
{
	FBXMeshLoader::LoadFBXFile(filePath, m_scene, outMeshes);
	for (auto it = outMeshes.begin(); it != outMeshes.end(); it++)
	{
		(*it)->SetMaterialPBR(pDefaultMaterial);
		m_scene->m_primitives.push_back(*it);
		m_scene->m_objects.push_back(*it);

		// 将所有FBXMeshes为nullptr的全算作root的子节点（场景内最外一层）
		if ((*it)->GetParent() == nullptr)
			(*it)->SetParent(m_scene->m_pRootObject);
	}
	return true;
}

std::shared_ptr<NXCamera> SceneManager::CreateCamera(const std::string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up)
{
	auto p = std::make_shared<NXCamera>();
	p->Init(FovY, zNear, zFar, eye, at, up);
	m_scene->m_pMainCamera = p;
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

std::shared_ptr<NXPBRMaterial> SceneManager::CreatePBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR)
{
	auto pMat = std::make_shared<NXPBRMaterial>(albedo, metallic, roughness, reflectivity, refractivity, IOR);
	m_scene->m_pbrMaterials.push_back(pMat);
	return pMat;
}

std::shared_ptr<NXPBRPointLight> SceneManager::CreatePBRPointLight(const Vector3& position, const Vector3& intensity)
{
	auto pLight = std::make_shared<NXPBRPointLight>(position, intensity);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

std::shared_ptr<NXPBRDistantLight> SceneManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& radiance)
{
	auto bound = m_scene->GetBoundingSphere();
	auto pLight = std::make_shared<NXPBRDistantLight>(direction, radiance, bound.Center, bound.Radius);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

std::shared_ptr<NXPBRTangibleLight> SceneManager::CreatePBRTangibleLight(const std::shared_ptr<NXPrimitive>& pPrimitive, const Vector3& radiance)
{
	auto pLight = std::make_shared<NXPBRTangibleLight>(pPrimitive, radiance);
	pPrimitive->SetTangibleLight(pLight);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

std::shared_ptr<NXPBREnvironmentLight> SceneManager::CreatePBREnvironmentLight(const std::shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Intensity)
{
	auto bound = m_scene->GetBoundingSphere();
	auto pLight = std::make_shared<NXPBREnvironmentLight>(pCubeMap, Intensity, bound.Center, bound.Radius);
	pCubeMap->SetEnvironmentLight(pLight);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

bool SceneManager::BindParent(std::shared_ptr<NXObject> pParent, std::shared_ptr<NXObject> pChild)
{
	if (!pChild || !pParent)
		return false;

	pChild->SetParent(pParent);
	return true;
}

