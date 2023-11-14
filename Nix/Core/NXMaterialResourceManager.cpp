#include "BaseDefs/NixCore.h"
#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"
#include "NXSSSDiffuseProfile.h"

void NXMaterialResourceManager::Init()
{
	m_pLoadingMaterial = new NXEasyMaterial("LoadingMaterial", "./Resource/loading.png");
	m_pErrorMaterial = new NXEasyMaterial("ErrorMaterial", "./Resource/error.dds");

	RegisterMaterial(m_pLoadingMaterial);
	RegisterMaterial(m_pErrorMaterial);

	m_defaultDiffuseProfile = new NXSSSDiffuseProfile();
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
	// 1. 遍历所有的 sss 材质，每出现一个新的 sss profile 路径，将该路径添加到 m_sssProfileGBufferIndexMap 中
	//	  a) 如果出现无效路径或者 profile 读取失败，将一个空路径Hash（"0"）添加到 m_sssProfileGBufferIndexMap。

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
			auto& pProfile = GetOrAddSSSProfile(path); // 维护一下 SSSProfileMap，目的是确保里面一定有当前帧绘制的所有材质
			PathHashValue pathHash = std::filesystem::hash_value(path);

			// 是否是没有记录过的新路径？
			if (m_sssProfileGBufferIndexMap.find(pathHash) == m_sssProfileGBufferIndexMap.end() || 
				m_sssProfileGBufferIndexMap[pathHash] == INVALID_SSS_PROFILE)
			{
				// 是，就存到 m_cbDiffuseProfileData 中
				AdjustDiffuseProfileRenderData(pathHash, sssGBufferIndex);

				// 同步保存这个 sss profile 在 GBuffer 的索引编号
				m_sssProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

				if (sssGBufferIndex >= 16)
					break; // 超过 16 个 profile，GBufferIndex 用完了，不再继续遍历
			}

			// 设置当前材质的 sss profile 在 GBuffer 的索引编号
			pCustomMat->SetGBufferIndexInternal(m_sssProfileGBufferIndexMap[pathHash]);
		}
	}
}

void NXMaterialResourceManager::AdjustDiffuseProfileRenderData(PathHashValue pathHash, UINT sssGBufferIndex)
{
	bool isInvalidProfile = pathHash == std::filesystem::hash_value("") || m_sssProfilesMap.find(pathHash) == m_sssProfilesMap.end();
	auto& pProfile = isInvalidProfile ? m_defaultDiffuseProfile : m_sssProfilesMap[pathHash];

	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatter = pProfile->GetScatter();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].scatterStrength = pProfile->GetScatterStrength();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmit = pProfile->GetTransmit();
	m_cbDiffuseProfileData.sssProfData[sssGBufferIndex].transmitStrength = pProfile->GetTransmitStrength();
}
