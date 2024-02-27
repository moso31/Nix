#include "BaseDefs/NixCore.h"
#include "Global.h"
#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"
#include "NXSSSDiffuseProfile.h"
#include "NXAllocatorManager.h"

void NXMaterialResourceManager::Init()
{
	m_pLoadingMaterial = new NXEasyMaterial("LoadingMaterial", "./Resource/loading.png");
	m_pErrorMaterial = new NXEasyMaterial("ErrorMaterial", "./Resource/error.dds");

	RegisterMaterial(m_pLoadingMaterial);
	RegisterMaterial(m_pErrorMaterial);

	m_defaultDiffuseProfile = new NXSSSDiffuseProfile();

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbDiffuseProfile.Get(i));
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
	// 此方法负责每帧维护 SSSProfile、Mesh、Material 之间的映射关系
	// 遍历所有的 sss 材质，每出现一个新的 sss profile 路径，将该路径添加到 m_sssProfileGBufferIndexMap 中
	//	  如果出现无效路径或者 profile 读取失败，将一个空路径Hash（"0"）添加到 m_sssProfileGBufferIndexMap。
	
	// 2023.11.14：Nix 现在没有视锥剔除，将来如果有了必然需要重写这里的逻辑，但现在就先这样吧

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

			// 是否是本次遍历过程中 没有记录过的新路径？
			if (m_sssProfileGBufferIndexMap.find(pathHash) == m_sssProfileGBufferIndexMap.end() || 
				m_sssProfileGBufferIndexMap[pathHash] == INVALID_SSS_PROFILE)
			{
				// 是，就存到 m_cbDiffuseProfileData 中
				AdjustDiffuseProfileRenderData(pathHash, sssGBufferIndex);

				// 同时保存这个 sss profile 在 GBuffer 的索引编号
				m_sssProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

				// 若超过 16 个 profile，GBufferIndex 用完了，没有继续遍历的必要
				if (sssGBufferIndex >= 16) break; 
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

	// 2023.11.21 
	// 原paper只是明确指出了"Burley SSS 的 s 值应该与 散射距离 成反比例"，但没有说具体的比例多少合适。
	// 所以现阶段先使用 基于引擎当前环境测的一个比较合理的scaleFactor。
	float scaleFactor = 0.01f;
	Vector3 scatterDistance = pProfile->GetScatter() * pProfile->GetScatterDistance() * scaleFactor;
	float maxScatterDistance = scatterDistance.MaxComponent();
	
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].scatterParam = scatterDistance.Reciprocal();
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].maxScatterDist = 1.0f / maxScatterDistance; // is rcp of dist actually!
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].transmit = pProfile->GetTransmit();
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].transmitStrength = pProfile->GetTransmitStrength();

	NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(m_cbDiffuseProfile.Current());
}
