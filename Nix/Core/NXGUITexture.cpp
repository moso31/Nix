#include "NXGUITexture.h"
#include <filesystem>
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
		SetImage(m_strImgPath);
	}

	ImGui::End();
}

void NXGUITexture::Release()
{
	SafeDelete(m_pTexImage);
}

void NXGUITexture::SetImage(const std::filesystem::path& strImgPath)
{
	m_strImgPath = strImgPath;

	SafeDelete(m_pTexImage);
	m_pTexImage = NXResourceManager::GetInstance()->CreateTexture2D("NXGUITexture Preview Image", strImgPath, m_bGenerateMipMap, m_bInvertNormalY, (NXTextureType)m_nTexType);
	m_pTexImage->AddSRV();
}