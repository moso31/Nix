#include "NXGUITexture.h"
#include <filesystem>
#include "NXGUICommon.h"
#include "NXResourceManager.h"

NXGUITexture::NXGUITexture() :
	m_pTexImage(nullptr),
	m_bGenerateMipMap(false),
	m_bInvertNormalY(false),
	m_nTexType(0)
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

	bool bTextureChanged = false;

	ImGui::Checkbox("Generate mip map##Texture", &m_bGenerateMipMap);
	ImGui::Checkbox("Invert normal Y##Texture", &m_bInvertNormalY);

	static const char* items[] = { "Default", "Normal map" };
	ImGui::Combo("Texture type##Texture", &m_nTexType, items, IM_ARRAYSIZE(items));

	if (m_nTexType == 1)
	{
		bool x = 0;
		ImGui::Checkbox("sRGB##Texture", &x);
	}

	if (ImGui::Button("Apply##Texture"))
	{
		NXTextureInfoData info;
		info.eTexType = (NXTextureType)m_nTexType;
		info.bGenerateMipMap = m_bGenerateMipMap;
		info.bInvertNormalY = m_bInvertNormalY;
		info.bCubeMap = false;
		info.bSRGB = false;

		SetImage(m_strImgPath, info);
		NXGUICommon::SaveTextureInfoFile(m_pTexImage, info);
	}

	ImGui::End();
}

void NXGUITexture::Release()
{
	SafeDelete(m_pTexImage);
}

void NXGUITexture::SetImage(const std::filesystem::path& path, const NXTextureInfoData& texInfoData)
{
	m_strImgPath = path;

	SafeDelete(m_pTexImage);
	m_pTexImage = NXResourceManager::GetInstance()->CreateTexture2D("NXGUITexture Preview Image", path, texInfoData.bGenerateMipMap, texInfoData.bInvertNormalY, (NXTextureType)texInfoData.eTexType);
	m_pTexImage->AddSRV();

	m_bGenerateMipMap = texInfoData.bGenerateMipMap;
	m_bInvertNormalY = texInfoData.bInvertNormalY;
	m_nTexType = texInfoData.eTexType;
}
