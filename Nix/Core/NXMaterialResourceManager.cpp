#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"

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

void NXMaterialResourceManager::OnReload()
{
	for (NXMaterial* pMat : m_pMaterialArray)
	{
		if (!pMat) continue;
		if (pMat->GetReloadingState() == NXMaterialReloadingState::Material_StartReload)
		{
			//pMat->();
		}
	}
}

void NXMaterialResourceManager::Release()
{
	for (auto pMat : m_pMaterialArray) SafeRelease(pMat);
}
