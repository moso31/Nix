#include "BaseDefs/NixCore.h"
#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"
#include "NXSSSDiffuseProfile.h"

void NXMaterialResourceManager::InitCommonMaterial()
{
	// ps: ֱ�ӽ���ʼ������д�ڹ��캯������
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
	// ����Ѿ����ڴ���ֱ���þ�����
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
	if (m_SSSProfilesMap.find(pathHash) != m_SSSProfilesMap.end())
		return m_SSSProfilesMap[pathHash];

	if (sssProfFilePath.extension().string() == ".nssprof")
	{
		// 2023.11.10 
		// TODO������û���ж�*.nssprof�ļ��ڵ�data�Ƿ�����Ч�ģ�Ŀǰ��ʱĬ�ϲ��������
		Ntr<NXSSSDiffuseProfile> pSSSProfile(new NXSSSDiffuseProfile());
		pSSSProfile->SetFilePath(sssProfFilePath);
		pSSSProfile->Deserialize();

		// ��ӵ� map ��ά�� GBufferIndexMap
		m_SSSProfilesMap[pathHash] = pSSSProfile;
		return pSSSProfile;
	}

	return nullptr;
}

void NXMaterialResourceManager::OnReload()
{
	// �ͷŲ��õĲ���
	ReleaseUnusedMaterials();

	// ÿ֡����Ҫ��� sssProfileGBufferIndexMap �����������¼���
	AdjustSSSProfileMapToGBufferIndex();
}

void NXMaterialResourceManager::Release()
{
	for (auto pMat : m_pMaterialArray) SafeRelease(pMat);
}

void NXMaterialResourceManager::ReleaseUnusedMaterials()
{
	for (auto mat : m_pUnusedMaterials) SafeRelease(mat);
	m_pUnusedMaterials.clear();
}

void NXMaterialResourceManager::AdjustSSSProfileMapToGBufferIndex()
{
	for (auto& [_, idx] : m_SSSProfileGBufferIndexMap) idx = -1;

	UINT8 sssGBufferIndex = 0;
	for (auto pMat : m_pMaterialArray)
	{
		auto pCustomMat = pMat->IsCustomMat();
		if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
		{
			PathHashValue pathHash = std::filesystem::hash_value(pCustomMat->GetSSSProfilePath());
			if (m_SSSProfileGBufferIndexMap.find(pathHash) == m_SSSProfileGBufferIndexMap.end() || m_SSSProfileGBufferIndexMap[pathHash] == -1)
				m_SSSProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

			pCustomMat->SetGBufferIndexInternal(m_SSSProfileGBufferIndexMap[pathHash]);
		}
	}
}
