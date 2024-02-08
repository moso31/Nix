#include "BaseDefs/DearImGui.h"

#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"
#include "NXGUIInspector.h"

NXGUITexture::NXGUITexture()
{
}

void NXGUITexture::Render()
{
	ImGui::Text("Texture");

	if (m_pTexImage.IsNull())
	{ 
		return;
	}

	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	ImGui::Image(ImTextureID(m_pTexImage->GetSRV()), ImVec2(fTexSize, fTexSize));

	ImGui::Checkbox("Generate mip map##Texture", &m_texData.m_bGenerateMipMap);
	ImGui::Checkbox("Invert normal Y##Texture", &m_texData.m_bInvertNormalY);

	const char* strTextureTypes[] = { "Raw", "sRGB", "Linear", "Normal Map" };
	int nTexType = (int)m_texData.m_textureType;
	if (ImGui::Combo("Texture type##Texture", &nTexType, strTextureTypes, IM_ARRAYSIZE(strTextureTypes)))
	{
		m_texData.m_textureType = (NXTextureMode)(nTexType);
	}

	if (ImGui::Button("Apply##Texture"))
	{
		m_pTexImage->SetSerializationData(m_texData);

		// ±£´æNXInfoÎÄ¼þ
		m_pTexImage->Serialize();
		m_pTexImage->MarkReload();
	}
}

void NXGUITexture::Release()
{
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	if (m_pTexImage.IsValid() && path == m_pTexImage->GetFilePath())
		return;

	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
	m_texData = m_pTexImage->GetSerializationData();
}
