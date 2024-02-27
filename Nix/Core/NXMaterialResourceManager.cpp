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
	// �˷�������ÿ֡ά�� SSSProfile��Mesh��Material ֮���ӳ���ϵ
	// �������е� sss ���ʣ�ÿ����һ���µ� sss profile ·��������·����ӵ� m_sssProfileGBufferIndexMap ��
	//	  ���������Ч·������ profile ��ȡʧ�ܣ���һ����·��Hash��"0"����ӵ� m_sssProfileGBufferIndexMap��
	
	// 2023.11.14��Nix ����û����׶�޳�������������˱�Ȼ��Ҫ��д������߼��������ھ���������

	const static UINT8 INVALID_SSS_PROFILE = 0xff;
	for (auto& [_, idx] : m_sssProfileGBufferIndexMap) idx = INVALID_SSS_PROFILE;

	UINT8 sssGBufferIndex = 0;
	for (auto pMat : m_pMaterialArray)
	{
		auto pCustomMat = pMat->IsCustomMat();
		if (pCustomMat && pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
		{
			auto& path = pCustomMat->GetSSSProfilePath();
			auto& pProfile = GetOrAddSSSProfile(path); // ά��һ�� SSSProfileMap��Ŀ����ȷ������һ���е�ǰ֡���Ƶ����в���
			PathHashValue pathHash = std::filesystem::hash_value(path);

			// �Ƿ��Ǳ��α��������� û�м�¼������·����
			if (m_sssProfileGBufferIndexMap.find(pathHash) == m_sssProfileGBufferIndexMap.end() || 
				m_sssProfileGBufferIndexMap[pathHash] == INVALID_SSS_PROFILE)
			{
				// �ǣ��ʹ浽 m_cbDiffuseProfileData ��
				AdjustDiffuseProfileRenderData(pathHash, sssGBufferIndex);

				// ͬʱ������� sss profile �� GBuffer ���������
				m_sssProfileGBufferIndexMap[pathHash] = sssGBufferIndex++;

				// ������ 16 �� profile��GBufferIndex �����ˣ�û�м��������ı�Ҫ
				if (sssGBufferIndex >= 16) break; 
			}

			// ���õ�ǰ���ʵ� sss profile �� GBuffer ���������
			pCustomMat->SetGBufferIndexInternal(m_sssProfileGBufferIndexMap[pathHash]);
		}
	}
}

void NXMaterialResourceManager::AdjustDiffuseProfileRenderData(PathHashValue pathHash, UINT sssGBufferIndex)
{
	bool isInvalidProfile = pathHash == std::filesystem::hash_value("") || m_sssProfilesMap.find(pathHash) == m_sssProfilesMap.end();
	auto& pProfile = isInvalidProfile ? m_defaultDiffuseProfile : m_sssProfilesMap[pathHash];

	// 2023.11.21 
	// ԭpaperֻ����ȷָ����"Burley SSS �� s ֵӦ���� ɢ����� �ɷ�����"����û��˵����ı������ٺ��ʡ�
	// �����ֽ׶���ʹ�� �������浱ǰ�������һ���ȽϺ����scaleFactor��
	float scaleFactor = 0.01f;
	Vector3 scatterDistance = pProfile->GetScatter() * pProfile->GetScatterDistance() * scaleFactor;
	float maxScatterDistance = scatterDistance.MaxComponent();
	
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].scatterParam = scatterDistance.Reciprocal();
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].maxScatterDist = 1.0f / maxScatterDistance; // is rcp of dist actually!
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].transmit = pProfile->GetTransmit();
	m_cbDiffuseProfile.Current().data.sssProfData[sssGBufferIndex].transmitStrength = pProfile->GetTransmitStrength();

	NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(m_cbDiffuseProfile.Current());
}
