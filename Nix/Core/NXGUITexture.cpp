#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"

NXGUITexture::NXGUITexture() :
	m_pTexInfo(nullptr),
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

	if (m_pTexInfo)
	{
		bool bGenerateMipMap = m_pTexInfo->bGenerateMipMap;
		if (ImGui::Checkbox("Generate mip map##Texture", &bGenerateMipMap))
		{
			m_pTexInfo->bGenerateMipMap = bGenerateMipMap;
		}

		bool bInvertNormalY = m_pTexInfo->bInvertNormalY;
		if (ImGui::Checkbox("Invert normal Y##Texture", &bInvertNormalY))
		{
			m_pTexInfo->bInvertNormalY = bInvertNormalY;
		}

		int nTexType = m_pTexInfo->nTexType;
		static const char* items[] = { "Default", "Normal map" };
		if (ImGui::Combo("Texture type##Texture", &nTexType, items, IM_ARRAYSIZE(items)))
		{
			m_pTexInfo->nTexType = nTexType;
		}

		if (m_pTexInfo->nTexType == 0)
		{
			bool bSRGB = m_pTexInfo->bSRGB;
			if (ImGui::Checkbox("sRGB##Texture", &bSRGB))
			{
				m_pTexInfo->bSRGB = bSRGB;
			}
		}

		if (ImGui::Button("Apply##Texture"))
		{
			// ±£´æNXInfoÎÄ¼þ
			NXResourceManager::GetInstance()->SaveTextureInfo(m_pTexInfo, m_pTexImage->GetFilePath());
			m_pTexImage->OnReload();

			SetImage(m_pTexImage->GetFilePath());
		}
	}

	ImGui::End();
}

void NXGUITexture::Release()
{
	if (m_pTexImage) m_pTexImage->RemoveRef();
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	NXTexture2D* pOldImage = m_pTexImage;
	if (pOldImage) pOldImage->RemoveRef();
	m_pTexInfo = nullptr;

	if (path.empty())
		return;

	m_pTexImage = NXResourceManager::GetInstance()->CreateTexture2D("NXGUITexture Preview Image", path);
	if (m_pTexImage)
	{
		m_pTexInfo = m_pTexImage->GetTextureNXInfo();
	}
}
