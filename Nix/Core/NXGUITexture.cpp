#include "NXGUITexture.h"
#include <filesystem>
#include <fstream>
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"

NXGUITexture::NXGUITexture() :
	m_pTexNXInfo(nullptr),
	m_pTexImage(nullptr)
{
}

void NXGUITexture::Render()
{
	ImGui::Begin("Texture");

	if (!m_pTexImage)
	{
		ImGui::End();
		return;
	}

	ImGui::Image(ImTextureID(m_pTexImage->GetSRV()), ImVec2(200.0f, 200.0f));

	bool bGenerateMipMap = m_pTexNXInfo->bGenerateMipMap;
	if (ImGui::Checkbox("Generate mip map##Texture", &bGenerateMipMap))
	{
		m_pTexNXInfo->bGenerateMipMap = bGenerateMipMap;
	}

	bool bInvertNormalY = m_pTexNXInfo->bInvertNormalY;
	if (ImGui::Checkbox("Invert normal Y##Texture", &bInvertNormalY))
	{
		m_pTexNXInfo->bInvertNormalY = bInvertNormalY;
	}

	int nTexType = m_pTexNXInfo->nTexType;
	static const char* items[] = { "Default", "Normal map" };
	if (ImGui::Combo("Texture type##Texture", &nTexType, items, IM_ARRAYSIZE(items)))
	{
		m_pTexNXInfo->nTexType = nTexType;
	}

	if (m_pTexNXInfo->nTexType == 0)
	{
		bool bSRGB = m_pTexNXInfo->bSRGB;
		if (ImGui::Checkbox("sRGB##Texture", &bSRGB))
		{
			m_pTexNXInfo->bSRGB = bSRGB;
		}
	}

	if (ImGui::Button("Apply##Texture"))
	{
		SaveTextureNXInfo(); // 保存NXInfo文件

		ReloadTexture();

		SetImage(m_strImgPath);
	}

	ImGui::End();
}

void NXGUITexture::Release()
{
	if (m_pTexImage) m_pTexImage->RemoveRef();
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	m_strImgPath = path;

	NXTexture2D* pOldImage = m_pTexImage;
	if (pOldImage) pOldImage->RemoveRef();

	m_pTexImage = NXResourceManager::GetInstance()->CreateTexture2D("NXGUITexture Preview Image", path);
	if (m_pTexImage)
	{
		m_pTexNXInfo = m_pTexImage->LoadTextureNXInfo(path);
		if (!m_pTexNXInfo)
			m_pTexNXInfo = new TextureNXInfo();
	}
}

void NXGUITexture::SaveTextureNXInfo()
{
	auto path = m_pTexImage->GetFilePath();
	if (path.empty())
		return;

	std::string strPathInfo = path.string() + ".nxInfo";

	std::ofstream ofs(strPathInfo, std::ios::binary);

	// 文件格式：
	// 纹理文件路径的哈希
	// (int)TexFormat, Width, Height, Arraysize, Miplevel
	// (int)TextureType, (int)IsSRGB, (int)IsInvertNormalY, (int)IsGenerateCubeMap, (int)IsCubeMap

	size_t pathHashValue = std::filesystem::hash_value(path);
	ofs << pathHashValue << std::endl;

	m_pTexNXInfo->TexFormat = m_pTexImage->GetFormat();
	m_pTexNXInfo->Width = m_pTexImage->GetWidth();
	m_pTexNXInfo->Height = m_pTexImage->GetHeight();

	ofs << m_pTexNXInfo->TexFormat << ' ' << m_pTexNXInfo->Width << ' ' << m_pTexNXInfo->Height << std::endl;
	ofs << m_pTexNXInfo->nTexType << ' ' << (int)m_pTexNXInfo->bSRGB << ' ' << (int)m_pTexNXInfo->bInvertNormalY << ' ' << (int)m_pTexNXInfo->bGenerateMipMap << ' ' << (int)m_pTexNXInfo->bCubeMap << std::endl;

	ofs.close();
}

void NXGUITexture::ReloadTexture()
{
	if (m_pTexImage)
		m_pTexImage->MakeDirty();

	for (auto pMat : m_pTexImage->GetRefMaterials())
	{
		pMat->ReloadTextures();
	}
}
