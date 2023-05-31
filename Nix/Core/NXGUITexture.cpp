#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"

NXGUITexture::NXGUITexture() :
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

	float fTexSize = ImGui::GetContentRegionAvail().x - 50.0f;
	ImGui::Image(ImTextureID(m_pTexImage->GetSRV()), ImVec2(fTexSize, fTexSize));

	bool bGenerateMipMap = m_pTexImage->GetIsGenerateMipMap();
	if (ImGui::Checkbox("Generate mip map##Texture", &bGenerateMipMap))
	{
		m_pTexImage->SetIsGenerateMipMap(bGenerateMipMap);
	}

	bool bInvertNormalY = m_pTexImage->GetIsInvertNormalY();
	if (ImGui::Checkbox("Invert normal Y##Texture", &bInvertNormalY))
	{
		m_pTexImage->SetIsInvertNormalY(bInvertNormalY);
	}

	int nTexType = (int)m_pTexImage->GetTextureType();
	if (ImGui::Combo("Texture type##Texture", &nTexType, g_strNXTextureType + 1, (int)NXTextureType::Count - 1))
	{
		m_pTexImage->SetTextureType(nTexType);
	}

	if (ImGui::Button("Apply##Texture"))
	{
		// 保存NXInfo文件
		m_pTexImage->Serialize();
		m_pTexImage->MarkReload();
	}
	ImGui::End();
}

void NXGUITexture::Release()
{
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	// 如果路径不合法，不显示纹理信息
	if (path.empty())
	{
		m_pTexImage = nullptr;
		return;
	}

	if (m_pTexImage)
	{
		// 如果路径相同，无需重新加载
		if (path == m_pTexImage->GetFilePath())
			return;

		// 路径不同，释放旧的纹理，并且不显示纹理信息
		m_pTexImage->RemoveRef();
	}

	// 如果和之前的路径不同，就加载新的纹理
	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
}
