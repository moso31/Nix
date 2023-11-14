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
	if (m_sssProfilesMap.find(pathHash) != m_sssProfilesMap.end())
		return m_sssProfilesMap[pathHash];

	if (sssProfFilePath.extension().string() == ".nssprof")
	{
		// 2023.11.10 
		// TODO������û���ж�*.nssprof�ļ��ڵ�data�Ƿ�����Ч�ģ�Ŀǰ��ʱĬ�ϲ��������
		Ntr<NXSSSDiffuseProfile> pSSSProfile(new NXSSSDiffuseProfile());
		pSSSProfile->SetFilePath(sssProfFilePath);
		pSSSProfile->Deserialize();

		// ��ӵ� map ��ά�� GBufferIndexMap
		m_sssProfilesMap[pathHash] = pSSSProfile;
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
	// ����������ά������
	// m_sssProfileGBufferIndexMap ����洢 SSSProfile ·�� �� GBufferIndex ��ӳ�䣬
	// ��Ⱦ GBuffer ʱ ��Ҫ������ȷ������ʹ�õ� GBufferIndex
	// m_sssProfileRenderList ����洢 GBufferIndex �� SSSProfile ��ӳ�䣬
	// ��Ⱦ 3S pass ʱ ��Ҫ������ȷ��ʵ�ʵ� SSSProfile ����
	
	// 2023.11.14��Nix ����û����׶�޳�������������˱�Ȼ��Ҫ��д�⡭��������������

	const static UINT8 INVALID_SSS_PROFILE = 0xff;
	for (auto& [_, idx] : m_sssProfileGBufferIndexMap) idx = INVALID_SSS_PROFILE;

	UINT8 sssGBufferIndex = 0;
	for (auto pMat : m_pMaterialArray)
	{
		auto pCustomMat = pMat->IsCustomMat();
		if (pCustomMat && pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
		{
			auto& path = pCustomMat->GetSSSProfilePath();
			if (path.empty()) continue;

			auto& pProfile = GetOrAddSSSProfile(path); // ά��һ�� SSSProfileMap��ȷ������һ���е�ǰ֡�Ĳ���
			PathHashValue pathHash = std::filesystem::hash_value(path);

			if (m_sssProfileGBufferIndexMap.find(pathHash) == m_sssProfileGBufferIndexMap.end() || 
				m_sssProfileGBufferIndexMap[pathHash] == INVALID_SSS_PROFILE)
			{
				AdjustDiffuseProfileRenderData(pathHash, sssGBufferIndex);

				m_sssProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

				if (sssGBufferIndex >= 16)
					break; // ���� 16 �� profile��GBufferIndex �����ˣ����ټ�������
			}

			pCustomMat->SetGBufferIndexInternal(m_sssProfileGBufferIndexMap[pathHash]);
		}
	}
}

void NXMaterialResourceManager::AdjustDiffuseProfileRenderData(PathHashValue pathHash, UINT sssGBufferIndex)
{
	// m_sssProfilesMap ����exist��飬����߼�����ȷ�� m_sssProfilesMap[pathHash] ��Ȼ����
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatter = m_sssProfilesMap[pathHash]->GetScatter();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatterStrength = m_sssProfilesMap[pathHash]->GetScatterStrength();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmit = m_sssProfilesMap[pathHash]->GetTransmit();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmitStrength = m_sssProfilesMap[pathHash]->GetTransmitStrength();
}
