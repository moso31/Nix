#include "SceneManager.h"
#include "NSFirstPersonalCamera.h"
#include "FBXMeshLoader.h"
#include "NXCubeMap.h"

SceneManager::SceneManager()
{
}

SceneManager::SceneManager(const shared_ptr<NXScene>& pScene) :
	m_scene(pScene)
{
}

SceneManager::~SceneManager()
{
}

shared_ptr<NXScript> SceneManager::CreateScript(const NXScriptType scriptType, const shared_ptr<NXObject>& pObject)
{
	switch (scriptType)
	{
	case NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA:
	{
		auto pScript = make_shared<NSFirstPersonalCamera>();
		//m_scene->GetScripts().push_back(pScript);
		return pScript;
	}
	default:
		return nullptr;
	}
}

shared_ptr<NXListener> SceneManager::AddEventListener(const NXEventType eventType, const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc)
{
	auto pListener = make_shared<NXListener>(pObject, pFunc);
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

shared_ptr<NXBox> SceneManager::CreateBox(const string& name, const float width, const float height, const float length, const shared_ptr<NXMaterial>& material, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = make_shared<NXBox>();
	p->SetName(name);
	p->Init(width, height, length);
	p->SetMaterial(material);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXSphere> SceneManager::CreateSphere(const string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const shared_ptr<NXMaterial>& material, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = make_shared<NXSphere>();
	p->SetName(name);
	p->Init(radius, segmentHorizontal, segmentVertical);
	p->SetMaterial(material);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXCylinder> SceneManager::CreateCylinder(const string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const shared_ptr<NXMaterial>& material, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = make_shared<NXCylinder>();
	p->SetName(name);
	p->Init(radius, length, segmentCircle, segmentLength);
	p->SetMaterial(material);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXPlane> SceneManager::CreatePlane(const string& name, const float width, const float height, const shared_ptr<NXMaterial>& material, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = make_shared<NXPlane>();
	p->SetName(name);
	p->Init(width, height);
	p->SetMaterial(material);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_scene->m_primitives.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

bool SceneManager::CreateFBXMeshes(const string& filePath, const shared_ptr<NXMaterial>& pDefaultMaterial, vector<shared_ptr<NXMesh>>& outMeshes)
{
	FBXMeshLoader::LoadFBXFile(filePath, m_scene, outMeshes);
	for (auto it = outMeshes.begin(); it != outMeshes.end(); it++)
	{
		(*it)->SetMaterial(pDefaultMaterial);
		m_scene->m_primitives.push_back(*it);
		m_scene->m_objects.push_back(*it);

		// 将所有FBXMeshes为nullptr的全算作root的子节点（场景内最外一层）
		if ((*it)->GetParent() == nullptr)
			(*it)->SetParent(m_scene->m_pRootObject);
	}
	return true;
}

shared_ptr<NXDirectionalLight> SceneManager::CreateDirectionalLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& direction)
{
	auto p = make_shared<NXDirectionalLight>();
	p->SetAmbient(ambient);
	p->SetDiffuse(diffuse);
	p->SetSpecular(Vector4(specular.x, specular.y, specular.z, specularW));
	p->SetDirection(direction);
	m_scene->m_lights.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXPointLight> SceneManager::CreatePointLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& position, const float range, const Vector3& attenuation)
{
	auto p = make_shared<NXPointLight>();
	p->SetAmbient(ambient);
	p->SetDiffuse(diffuse);
	p->SetSpecular(Vector4(specular.x, specular.y, specular.z, specularW));
	p->SetTranslation(position);
	p->SetRange(range);
	p->SetAtt(attenuation);
	m_scene->m_lights.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXSpotLight> SceneManager::CreateSpotLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& position, const float range, const Vector3& direction, const float spot, const Vector3& attenuation)
{
	auto p = make_shared<NXSpotLight>();
	p->SetAmbient(ambient);
	p->SetDiffuse(diffuse);
	p->SetSpecular(Vector4(specular.x, specular.y, specular.z, specularW));
	p->SetTranslation(position);
	p->SetRange(range);
	p->SetDirection(direction);
	p->SetSpot(spot);
	p->SetAtt(attenuation);
	m_scene->m_lights.push_back(p);
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXCamera> SceneManager::CreateCamera(const string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up)
{
	auto p = make_shared<NXCamera>();
	p->Init(FovY, zNear, zFar, eye, at, up);
	m_scene->m_mainCamera = p;
	m_scene->m_objects.push_back(p);
	p->SetParent(m_scene->m_pRootObject);
	return p;
}

shared_ptr<NXMaterial> SceneManager::CreateMaterial(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector4& specular, const float opacity, const Vector4& reflect)
{
	auto p = make_shared<NXMaterial>();
	p->SetAmbient(ambient);
	p->SetDiffuse(diffuse);
	p->SetSpecular(specular);
	p->SetOpacity(opacity);
	p->SetReflect(reflect);
	m_scene->m_materials.push_back(p);
	return p;
}

shared_ptr<NXPBRMaterial> SceneManager::CreatePBRMaterial(const Vector3& diffuse, const Vector3& specular, const Vector3& reflectivity, const Vector3& refractivity, float roughness, float IOR)
{
	auto pMat = make_shared<NXPBRMaterial>(diffuse, specular, reflectivity, refractivity, roughness, IOR);
	m_scene->m_pbrMaterials.push_back(pMat);
	return pMat;
}

shared_ptr<NXPBRLight> SceneManager::CreatePBRPointLight(const Vector3& position, const Vector3& intensity)
{
	auto pLight = make_shared<NXPBRPointLight>(position, intensity);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

shared_ptr<NXPBRLight> SceneManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& radiance)
{
	auto bound = m_scene->GetBoundingSphere();
	auto pLight = make_shared<NXPBRDistantLight>(direction, radiance, bound.Center, bound.Radius);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

shared_ptr<NXPBRLight> SceneManager::CreatePBRTangibleLight(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& radiance)
{
	auto pLight = make_shared<NXPBRTangibleLight>(pPrimitive, radiance);
	pPrimitive->SetTangibleLight(pLight);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

shared_ptr<NXPBRLight> SceneManager::CreatePBREnvironmentLight(const shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Intensity)
{
	auto bound = m_scene->GetBoundingSphere();
	auto pLight = make_shared<NXPBREnvironmentLight>(pCubeMap, Intensity, bound.Center, bound.Radius);
	pCubeMap->SetEnvironmentLight(pLight);
	m_scene->m_pbrLights.push_back(pLight);
	return pLight;
}

bool SceneManager::BindParent(shared_ptr<NXObject> pParent, shared_ptr<NXObject> pChild)
{
	if (!pChild || !pParent)
		return false;

	pChild->SetParent(pParent);
	return true;
}

