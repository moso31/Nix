#include "NXMaterialResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXHLSLGenerator.h"

void NXMaterialResourceManager::InitCommonMaterial()
{
	// 初始化一个默认材质，用于默认材质、切换材质时的过渡材质 等功能。
	//		ps: 直接将初始化纹理写在构造函数里了
	auto pDefaultMat = new NXEasyMaterial("DefaultMaterial", "./Resource/loading.png");
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

	//if (strType == "Standard")
	//	pNewMat = LoadStandardPBRMaterialFromFile(ifs, strName, strMatFilePath);
	//if (strType == "Translucent")
	//	pNewMat = LoadTranslucentPBRMaterialFromFile(ifs, strName, strMatFilePath);

	ifs.close();

	return pNewMat;
}

NXCustomMaterial* NXMaterialResourceManager::CreateCustomMaterial(const std::string& name, const std::filesystem::path& nslFilePath)
{
	auto pMat = new NXCustomMaterial(name, nslFilePath);
	pMat->LoadShaderCode();

	std::string strHLSLHead, strHLSLBody;
	std::vector<std::string> strHLSLFuncs;
	pMat->ConvertNSLToHLSL(strHLSLHead, strHLSLFuncs, strHLSLBody);

	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLFuncs, strHLSLBody, strGBufferShader, std::vector<NXHLSLCodeRegion>());

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
