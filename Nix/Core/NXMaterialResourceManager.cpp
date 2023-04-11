#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"

void NXMaterialResourceManager::InitCommonMaterial()
{
	// 初始化一个默认材质，用于默认材质、切换材质时的过渡材质 等功能。
	auto pDefaultMat = new NXPBRMaterialStandard("DefaultMaterial", Vector3(1.0f), Vector3(1.0f), 1.0f, 1.0f, 1.0f, "");
	pDefaultMat->SetTexAlbedo(g_defaultTex_white_wstr);
	pDefaultMat->SetTexNormal(g_defaultTex_normal_wstr);
	pDefaultMat->SetTexMetallic(g_defaultTex_white_wstr);
	pDefaultMat->SetTexRoughness(g_defaultTex_white_wstr);
	pDefaultMat->SetTexAO(g_defaultTex_white_wstr);
	RegisterMaterial(pDefaultMat);

	SafeRelease(m_pDefaultMaterial);
	m_pDefaultMaterial = pDefaultMat;
}

void NXMaterialResourceManager::RegisterMaterial(NXMaterial* newMaterial)
{
	m_pMaterialArray.push_back(newMaterial);
}

NXMaterial* NXMaterialResourceManager::FindMaterial(const std::filesystem::path& path)
{
	size_t matPathHash = std::filesystem::hash_value(path);

	for (auto& mat : m_pMaterialArray)
	{
		if (mat->GetFilePathHash() == matPathHash)
			return mat;
	}

	return nullptr;
}

void NXMaterialResourceManager::ReplaceMaterial(NXMaterial* oldMaterial, NXMaterial* newMaterial)
{
	std::replace(m_pMaterialArray.begin(), m_pMaterialArray.end(), oldMaterial, newMaterial);
}

NXMaterial* NXMaterialResourceManager::LoadFromNmatFile(const std::filesystem::path& matFilePath)
{
	std::string strMatFilePath = matFilePath.string().c_str();

	// 如果已经在内存里直接拿就行了
	NXMaterial* pNewMat = NXResourceManager::GetInstance()->GetMaterialManager()->FindMaterial(matFilePath);
	if (pNewMat) return pNewMat;

	// 否则需要读路径文件创建新材质
	std::ifstream ifs(strMatFilePath, std::ios::binary);

	if (!ifs.is_open())
		return nullptr;

	// 材质名称，材质类型
	std::string strName, strType;
	NXConvert::getline_safe(ifs, strName);
	NXConvert::getline_safe(ifs, strType);

	if (strType == "Standard")
		pNewMat = LoadStandardPBRMaterialFromFile(ifs, strName, strMatFilePath);
	if (strType == "Translucent")
		pNewMat = LoadTranslucentPBRMaterialFromFile(ifs, strName, strMatFilePath);

	ifs.close();

	return pNewMat;
}

NXMaterial* NXMaterialResourceManager::LoadStandardPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath)
{
	std::string strToNext;

	std::string strAlbedoTexPath;
	NXConvert::getline_safe(ifs, strAlbedoTexPath);
	if (NXConvert::IsMaterialDefaultPath(strAlbedoTexPath))
		strAlbedoTexPath = g_defaultTex_white_str;

	Vector3 albedo;
	ifs >> albedo.x >> albedo.y >> albedo.z;
	std::getline(ifs, strToNext);

	std::string strNormalTexPath;
	NXConvert::getline_safe(ifs, strNormalTexPath);
	if (NXConvert::IsMaterialDefaultPath(strNormalTexPath))
		strNormalTexPath = g_defaultTex_normal_str;

	Vector3 normal;
	ifs >> normal.x >> normal.y >> normal.z;
	std::getline(ifs, strToNext);

	std::string strMetallicTexPath;
	NXConvert::getline_safe(ifs, strMetallicTexPath);
	if (NXConvert::IsMaterialDefaultPath(strMetallicTexPath))
		strMetallicTexPath = g_defaultTex_white_str;

	float metallic;
	ifs >> metallic;
	std::getline(ifs, strToNext);

	std::string strRoughnessTexPath;
	NXConvert::getline_safe(ifs, strRoughnessTexPath);
	if (NXConvert::IsMaterialDefaultPath(strRoughnessTexPath))
		strRoughnessTexPath = g_defaultTex_white_str;

	float roughness;
	ifs >> roughness;
	std::getline(ifs, strToNext);

	std::string strAOTexPath;
	NXConvert::getline_safe(ifs, strAOTexPath);
	if (NXConvert::IsMaterialDefaultPath(strAOTexPath))
		strAOTexPath = g_defaultTex_white_str;

	float ao;
	ifs >> ao;
	std::getline(ifs, strToNext);

	return CreatePBRMaterialStandard(matName, albedo, normal, metallic, roughness, ao, NXConvert::s2ws(strAlbedoTexPath), NXConvert::s2ws(strNormalTexPath), NXConvert::s2ws(strMetallicTexPath), NXConvert::s2ws(strRoughnessTexPath), NXConvert::s2ws(strAOTexPath), matFilePath);
}

NXMaterial* NXMaterialResourceManager::LoadTranslucentPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath)
{
	std::string strToNext;

	std::string strAlbedoTexPath;
	NXConvert::getline_safe(ifs, strAlbedoTexPath);
	if (NXConvert::IsMaterialDefaultPath(strAlbedoTexPath))
		strAlbedoTexPath = g_defaultTex_white_str;

	Vector3 albedo;
	float opacity;
	ifs >> albedo.x >> albedo.y >> albedo.z >> opacity;
	std::getline(ifs, strToNext);

	std::string strNormalTexPath;
	NXConvert::getline_safe(ifs, strNormalTexPath);
	if (NXConvert::IsMaterialDefaultPath(strNormalTexPath))
		strNormalTexPath = g_defaultTex_normal_str;

	Vector3 normal;
	ifs >> normal.x >> normal.y >> normal.z;
	std::getline(ifs, strToNext);

	std::string strMetallicTexPath;
	NXConvert::getline_safe(ifs, strMetallicTexPath);
	if (NXConvert::IsMaterialDefaultPath(strMetallicTexPath))
		strMetallicTexPath = g_defaultTex_white_str;

	float metallic;
	ifs >> metallic;
	std::getline(ifs, strToNext);

	std::string strRoughnessTexPath;
	NXConvert::getline_safe(ifs, strRoughnessTexPath);
	if (NXConvert::IsMaterialDefaultPath(strRoughnessTexPath))
		strRoughnessTexPath = g_defaultTex_white_str;

	float roughness;
	ifs >> roughness;
	std::getline(ifs, strToNext);

	std::string strAOTexPath;
	NXConvert::getline_safe(ifs, strAOTexPath);
	if (NXConvert::IsMaterialDefaultPath(strAOTexPath))
		strAOTexPath = g_defaultTex_white_str;

	float ao;
	ifs >> ao;
	std::getline(ifs, strToNext);

	return CreatePBRMaterialTranslucent(matName, albedo, normal, metallic, roughness, ao, opacity, NXConvert::s2ws(strAlbedoTexPath), NXConvert::s2ws(strNormalTexPath), NXConvert::s2ws(strMetallicTexPath), NXConvert::s2ws(strRoughnessTexPath), NXConvert::s2ws(strAOTexPath), matFilePath);
}

NXPBRMaterialStandard* NXMaterialResourceManager::CreatePBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao,
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
	NXResourceManager::GetInstance()->GetMaterialManager()->RegisterMaterial(pMat);
	return pMat;
}

NXPBRMaterialTranslucent* NXMaterialResourceManager::CreatePBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity,
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
	NXResourceManager::GetInstance()->GetMaterialManager()->RegisterMaterial(pMat);
	return pMat;
}

NXCustomMaterial* NXMaterialResourceManager::CreateCustomMaterial(const std::string& name, const std::string& nslFilePath)
{
	auto pMat = new NXCustomMaterial();
	pMat->SetShaderFilePath("./shader/GBufferEx_Test.nsl");
	pMat->LoadShaderCode();
	pMat->Compile();
	NXResourceManager::GetInstance()->GetMaterialManager()->RegisterMaterial(pMat);
	return pMat;
}

void NXMaterialResourceManager::ReTypeMaterial(NXMaterial* srcMaterial, NXMaterialType destMaterialType)
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
	case PBR_SUBSURFACE:
	{
		auto newMaterial = new NXPBRMaterialSubsurface(srcMaterial->GetName(), Vector3(1.0f), Vector3(1.0f), 1.0f, 1.0f, 1.0f, 1.0f, Vector3(1.0f), srcMaterial->GetFilePath());
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

		NXResourceManager::GetInstance()->GetMaterialManager()->ReplaceMaterial(srcMaterial, destMaterial);

		m_pUnusedMaterials.push_back(srcMaterial);
	}
}

void NXMaterialResourceManager::OnReload()
{
	//for (NXMaterial* pMat : m_pMaterialArray)
	//{
	//	if (!pMat) continue;
	//	if (pMat->GetReloadingState() == NXMaterialReloadingState::Material_StartReload)
	//	{
	//		//pMat->();
	//	}
	//}

	for (auto mat : m_pUnusedMaterials)
	{
		SafeRelease(mat);
	}
	m_pUnusedMaterials.clear();
}

void NXMaterialResourceManager::Release()
{
	for (auto pMat : m_pMaterialArray) SafeRelease(pMat);
}
