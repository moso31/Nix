#include "BaseDefs/NixCore.h"
#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"
#include "NXSSSDiffuseProfile.h"

void NXMaterialResourceManager::InitCommonMaterial()
{
	// ps: 直接将初始化纹理写在构造函数里了
	m_pLoadingMaterial = new NXEasyMaterial("LoadingMaterial", "./Resource/loading.png");
	m_pErrorMaterial = new NXEasyMaterial("ErrorMaterial", "./Resource/error.dds");

	RegisterMaterial(m_pLoadingMaterial);
	RegisterMaterial(m_pErrorMaterial);
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

NXMaterial* NXMaterialResourceManager::LoadFromNSLFile(const std::filesystem::path& matFilePath)
{
	// 如果已经在内存里直接拿就行了
	NXMaterial* pNewMat = FindMaterial(matFilePath);
	if (!pNewMat) 
		pNewMat = CreateCustomMaterial("Hello", matFilePath);
	return pNewMat;
}

NXCustomMaterial* NXMaterialResourceManager::CreateCustomMaterial(const std::string& name, const std::filesystem::path& nslFilePath)
{
	auto pMat = new NXCustomMaterial(name, nslFilePath);
	pMat->LoadAndCompile(nslFilePath);

	RegisterMaterial(pMat);
	return pMat;
}

Ntr<NXSSSDiffuseProfile> NXMaterialResourceManager::GetOrAddSSSProfile(const std::filesystem::path& sssProfFilePath)
{
	size_t pathHash = std::filesystem::hash_value(sssProfFilePath);
	if (m_pSSSProfiles.find(pathHash) != m_pSSSProfiles.end())
	{
		return m_pSSSProfiles[pathHash];
	}

	if (sssProfFilePath.extension().string() == ".nssprof")
	{
		// 2023.11.10 
		// TODO：这里没有判断*.nssprof文件内的data是否是有效的，目前暂时默认不会出问题
		Ntr<NXSSSDiffuseProfile> pSSSProfile(new NXSSSDiffuseProfile());
		pSSSProfile->SetFilePath(sssProfFilePath);
		pSSSProfile->Deserialize();

		m_pSSSProfiles[pathHash] = pSSSProfile;
	}

	return nullptr;
}

void NXMaterialResourceManager::OnReload()
{
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
