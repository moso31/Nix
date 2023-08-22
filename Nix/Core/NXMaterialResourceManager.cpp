#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"

void NXMaterialResourceManager::InitCommonMaterial()
{
	// ps: 直接将初始化纹理写在构造函数里了
	m_pLoadingMaterial = new NXEasyMaterial("LoadingMaterial", "./Resource/loading.png");
	m_pErrorMaterial = new NXEasyMaterial("ErrorMaterial", "./Resource/error.png");

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
	NXMaterial* pNewMat = NXResourceManager::GetInstance()->GetMaterialManager()->FindMaterial(matFilePath);
	if (!pNewMat) 
		pNewMat = NXResourceManager::GetInstance()->GetMaterialManager()->CreateCustomMaterial("Hello", matFilePath);
	return pNewMat;
}

NXCustomMaterial* NXMaterialResourceManager::CreateCustomMaterial(const std::string& name, const std::filesystem::path& nslFilePath)
{
	auto pMat = new NXCustomMaterial(name, nslFilePath);
	pMat->LoadShaderCode();

	std::string strHLSLHead, strHLSLBody;
	std::vector<std::string> strHLSLFuncs;
	pMat->ConvertNSLToHLSL(strHLSLHead, strHLSLFuncs, strHLSLBody);

	std::vector<std::string> strHLSLTitles;
	strHLSLTitles.push_back("main()");
	for (int i = 0; i < strHLSLFuncs.size(); i++)
		strHLSLTitles.push_back(NXConvert::GetTitleOfFunctionData(strHLSLFuncs[i]));

	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLFuncs, strHLSLTitles, strHLSLBody, strGBufferShader, std::vector<NXHLSLCodeRegion>());

	std::string strErrMsgVS, strErrMsgPS;
	pMat->CompileShader(strGBufferShader, strErrMsgVS, strErrMsgPS);

	pMat->InitShaderResources();

	NXResourceManager::GetInstance()->GetMaterialManager()->RegisterMaterial(pMat);
	return pMat;
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
