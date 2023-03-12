#include "SceneManager.h"
#include <fstream>
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

NXMaterial* SceneManager::LoadFromNmatFile(const std::filesystem::path& matFilePath)
{
	std::string strMatFilePath = matFilePath.string().c_str();
	size_t pathHash = std::filesystem::hash_value(strMatFilePath);

	// 如果已经在内存里直接拿就行了
	NXMaterial* pNewMat = FindMaterial(pathHash);
	if (pNewMat) return pNewMat;

	// 否则需要读路径文件创建新材质
	std::ifstream ifs(strMatFilePath, std::ios::binary);

	if (!ifs.is_open())
		return nullptr;

	// 材质名称，材质类型
	std::string strName, strType;
	getline_safe(ifs, strName);
	getline_safe(ifs, strType);

	if (strType == "Standard") 
		pNewMat = LoadStandardPBRMaterialFromFile(ifs, strName, strMatFilePath);
	if (strType == "Translucent") 
		pNewMat = LoadTranslucentPBRMaterialFromFile(ifs, strName, strMatFilePath);

	ifs.close();

	return pNewMat;
}

NXMaterial* SceneManager::LoadStandardPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath)
{
	std::string strToNext;

	std::string strAlbedoTexPath;
	getline_safe(ifs, strAlbedoTexPath);
	if (IsDefaultPath(strAlbedoTexPath))
		strAlbedoTexPath = g_defaultTex_white_str;

	Vector3 albedo;
	ifs >> albedo.x >> albedo.y >> albedo.z;
	std::getline(ifs, strToNext);

	std::string strNormalTexPath;
	getline_safe(ifs, strNormalTexPath);
	if (IsDefaultPath(strNormalTexPath))
		strNormalTexPath = g_defaultTex_normal_str;

	Vector3 normal;
	ifs >> normal.x >> normal.y >> normal.z;
	std::getline(ifs, strToNext);

	std::string strMetallicTexPath;
	getline_safe(ifs, strMetallicTexPath);
	if (IsDefaultPath(strMetallicTexPath))
		strMetallicTexPath = g_defaultTex_white_str;

	float metallic;
	ifs >> metallic;
	std::getline(ifs, strToNext);

	std::string strRoughnessTexPath;
	getline_safe(ifs, strRoughnessTexPath);
	if (IsDefaultPath(strRoughnessTexPath))
		strRoughnessTexPath = g_defaultTex_white_str;

	float roughness;
	ifs >> roughness;
	std::getline(ifs, strToNext);

	std::string strAOTexPath;
	getline_safe(ifs, strAOTexPath);
	if (IsDefaultPath(strAOTexPath))
		strAOTexPath = g_defaultTex_white_str;

	float ao;
	ifs >> ao;
	std::getline(ifs, strToNext);

	return SceneManager::GetInstance()->CreatePBRMaterialStandard(matName, albedo, normal, metallic, roughness, ao, NXConvert::s2ws(strAlbedoTexPath), NXConvert::s2ws(strNormalTexPath), NXConvert::s2ws(strMetallicTexPath), NXConvert::s2ws(strRoughnessTexPath), NXConvert::s2ws(strAOTexPath), matFilePath);
}

NXMaterial* SceneManager::LoadTranslucentPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath)
{
	std::string strToNext;

	std::string strAlbedoTexPath;
	getline_safe(ifs, strAlbedoTexPath);
	if (IsDefaultPath(strAlbedoTexPath))
		strAlbedoTexPath = g_defaultTex_white_str;

	Vector3 albedo;
	float opacity;
	ifs >> albedo.x >> albedo.y >> albedo.z >> opacity;
	std::getline(ifs, strToNext);

	std::string strNormalTexPath;
	getline_safe(ifs, strNormalTexPath);
	if (IsDefaultPath(strNormalTexPath))
		strNormalTexPath = g_defaultTex_normal_str;

	Vector3 normal;
	ifs >> normal.x >> normal.y >> normal.z;
	std::getline(ifs, strToNext);

	std::string strMetallicTexPath;
	getline_safe(ifs, strMetallicTexPath);
	if (IsDefaultPath(strMetallicTexPath))
		strMetallicTexPath = g_defaultTex_white_str;

	float metallic;
	ifs >> metallic;
	std::getline(ifs, strToNext);

	std::string strRoughnessTexPath;
	getline_safe(ifs, strRoughnessTexPath);
	if (IsDefaultPath(strRoughnessTexPath))
		strRoughnessTexPath = g_defaultTex_white_str;

	float roughness;
	ifs >> roughness;
	std::getline(ifs, strToNext);

	std::string strAOTexPath;
	getline_safe(ifs, strAOTexPath);
	if (IsDefaultPath(strAOTexPath))
		strAOTexPath = g_defaultTex_white_str;

	float ao;
	ifs >> ao;
	std::getline(ifs, strToNext);

	return SceneManager::GetInstance()->CreatePBRMaterialTranslucent(matName, albedo, normal, metallic, roughness, ao, opacity, NXConvert::s2ws(strAlbedoTexPath), NXConvert::s2ws(strNormalTexPath), NXConvert::s2ws(strMetallicTexPath), NXConvert::s2ws(strRoughnessTexPath), NXConvert::s2ws(strAOTexPath), matFilePath);
}

NXPBRMaterialStandard* SceneManager::CreatePBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao,
	const std::wstring albedoTexFilePath,
	const std::wstring normalTexFilePath,
	const std::wstring metallicTexFilePath,
	const std::wstring roughnessTexFilePath,
	const std::wstring aoTexFilePath,
	const std::string& filePath)
{
	auto pMat = new NXPBRMaterialStandard(name, albedo, normal, metallic, roughness, ao, filePath);

	pMat->SetTexAlbedo(albedoTexFilePath);
	pMat->SetTexNormal(normalTexFilePath);
	pMat->SetTexMetallic(metallicTexFilePath);
	pMat->SetTexRoughness(roughnessTexFilePath);
	pMat->SetTexAO(aoTexFilePath);
	RegisterMaterial(pMat);
	return pMat;
}

NXPBRMaterialTranslucent* SceneManager::CreatePBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, 
	const std::wstring albedoTexFilePath, 
	const std::wstring normalTexFilePath, 
	const std::wstring metallicTexFilePath, 
	const std::wstring roughnessTexFilePath, 
	const std::wstring aoTexFilePath, 
	const std::string& filePath)
{
	auto pMat = new NXPBRMaterialTranslucent(name, albedo, normal, metallic, roughness, ao, opacity, filePath);

	pMat->SetTexAlbedo(albedoTexFilePath);
	pMat->SetTexNormal(normalTexFilePath);
	pMat->SetTexMetallic(metallicTexFilePath);
	pMat->SetTexRoughness(roughnessTexFilePath);
	pMat->SetTexAO(aoTexFilePath);
	RegisterMaterial(pMat);
	return pMat;
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

void SceneManager::ReTypeMaterial(NXMaterial* srcMaterial, NXMaterialType destMaterialType)
{
	if (srcMaterial->GetType() == destMaterialType)
		return;

	const std::wstring albedoTexFilePath = g_defaultTex_white_wstr;
	const std::wstring normalTexFilePath = g_defaultTex_normal_wstr;
	const std::wstring metallicTexFilePath = g_defaultTex_white_wstr;
	const std::wstring roughnessTexFilePath = g_defaultTex_white_wstr;
	const std::wstring aoTexFilePath = g_defaultTex_white_wstr;

	NXMaterial* destMaterial;
	switch (destMaterialType)
	{
	case PBR_STANDARD:
	{
		auto newMaterial = new NXPBRMaterialStandard(srcMaterial->GetName(), Vector3(1.0f), Vector3(1.0f), 1.0f, 1.0f, 1.0f, srcMaterial->GetFilePath());
		newMaterial->SetTexAlbedo(albedoTexFilePath);
		newMaterial->SetTexNormal(normalTexFilePath);
		newMaterial->SetTexMetallic(metallicTexFilePath);
		newMaterial->SetTexRoughness(roughnessTexFilePath);
		newMaterial->SetTexAO(aoTexFilePath);

		destMaterial = newMaterial;
		break;
	}
	case PBR_TRANSLUCENT:
	{
		auto newMaterial = new NXPBRMaterialTranslucent(srcMaterial->GetName(), Vector3(1.0f), Vector3(1.0f), 1.0f, 1.0f, 1.0f, 1.0f, srcMaterial->GetFilePath());
		newMaterial->SetTexAlbedo(albedoTexFilePath);
		newMaterial->SetTexNormal(normalTexFilePath);
		newMaterial->SetTexMetallic(metallicTexFilePath);
		newMaterial->SetTexRoughness(roughnessTexFilePath);
		newMaterial->SetTexAO(aoTexFilePath);

		destMaterial = newMaterial;
		break;
	}
	default:
		destMaterial = nullptr;
		break;
	}

	if (destMaterial)
	{
		for (auto pSubMesh : srcMaterial->GetRefSubMeshes())
		{
			if (pSubMesh)
			{
				pSubMesh->SetMaterial(destMaterial);
				destMaterial->AddSubMesh(pSubMesh);
			}
		}

		auto& sceneMats = s_pWorkingScene->m_materials;
		std::replace(sceneMats.begin(), sceneMats.end(), srcMaterial, destMaterial);

		NXResourceReloader::GetInstance()->MarkUnusedMaterial(srcMaterial);
	}
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

NXCubeMap* SceneManager::CreateCubeMap(const std::string name, const std::wstring filePath)
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

void SceneManager::RegisterMaterial(NXMaterial* newMaterial)
{
	s_pWorkingScene->m_materials.push_back(newMaterial);
}

void SceneManager::RegisterLight(NXPBRLight* newLight, NXObject* pParent)
{
	s_pWorkingScene->m_pbrLights.push_back(newLight);
}

NXMaterial* SceneManager::FindMaterial(size_t matPathHash)
{
	for (auto pMat : s_pWorkingScene->m_materials)
		if (pMat->GetFilePathHash() == matPathHash) return pMat;

	return nullptr;
}

void SceneManager::getline_safe(std::ifstream& ifs, std::string& s)
{
	std::getline(ifs, s);
	s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
	s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
}

bool SceneManager::IsDefaultPath(const std::string& s)
{
	return s == "?";
}