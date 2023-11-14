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
	if (m_sssProfilesMap.find(pathHash) != m_sssProfilesMap.end())
		return m_sssProfilesMap[pathHash];

	if (sssProfFilePath.extension().string() == ".nssprof")
	{
		// 2023.11.10 
		// TODO：这里没有判断*.nssprof文件内的data是否是有效的，目前暂时默认不会出问题
		Ntr<NXSSSDiffuseProfile> pSSSProfile(new NXSSSDiffuseProfile());
		pSSSProfile->SetFilePath(sssProfFilePath);
		pSSSProfile->Deserialize();

		// 添加到 map 并维护 GBufferIndexMap
		m_sssProfilesMap[pathHash] = pSSSProfile;
		return pSSSProfile;
	}

	return nullptr;
}

void NXMaterialResourceManager::OnReload()
{
	// 释放不用的材质
	ReleaseUnusedMaterials();

	// 每帧都需要清空 sssProfileGBufferIndexMap 的索引，重新计算
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
	// 本方法负责维护序列
	// m_sssProfileGBufferIndexMap 负责存储 SSSProfile 路径 和 GBufferIndex 的映射，
	// 渲染 GBuffer 时 需要依赖它确定材质使用的 GBufferIndex
	// m_sssProfileRenderList 负责存储 GBufferIndex 和 SSSProfile 的映射，
	// 渲染 3S pass 时 需要依赖它确定实际的 SSSProfile 数据
	
	// 2023.11.14：Nix 现在没有视锥剔除，将来如果有了必然需要重写这……但现在先这样

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

			auto& pProfile = GetOrAddSSSProfile(path); // 维护一下 SSSProfileMap，确保里面一定有当前帧的材质
			PathHashValue pathHash = std::filesystem::hash_value(path);

			if (m_sssProfileGBufferIndexMap.find(pathHash) == m_sssProfileGBufferIndexMap.end() || 
				m_sssProfileGBufferIndexMap[pathHash] == INVALID_SSS_PROFILE)
			{
				AdjustDiffuseProfileRenderData(pathHash, sssGBufferIndex);

				m_sssProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

				if (sssGBufferIndex >= 16)
					break; // 超过 16 个 profile，GBufferIndex 用完了，不再继续遍历
			}

			pCustomMat->SetGBufferIndexInternal(m_sssProfileGBufferIndexMap[pathHash]);
		}
	}
}

void NXMaterialResourceManager::AdjustDiffuseProfileRenderData(PathHashValue pathHash, UINT sssGBufferIndex)
{
	// m_sssProfilesMap 无需exist检查，外层逻辑负责确保 m_sssProfilesMap[pathHash] 必然存在
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatter = m_sssProfilesMap[pathHash]->GetScatter();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatterStrength = m_sssProfilesMap[pathHash]->GetScatterStrength();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmit = m_sssProfilesMap[pathHash]->GetTransmit();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmitStrength = m_sssProfilesMap[pathHash]->GetTransmitStrength();
}
