#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"

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

		
		int nTexType = (int)m_pTexInfo->eType;
		if (ImGui::Combo("Texture type##Texture", &nTexType, g_NXTextureType, (int)NXTextureType::Count))
		{
			m_pTexInfo->eType = (NXTextureType)nTexType;
		}

		if (ImGui::Button("Apply##Texture"))
		{
			// 保存NXInfo文件
			NXResourceManager::GetInstance()->GetTextureManager()->SaveTextureInfo(m_pTexInfo, m_pTexImage->GetFilePath());
			m_pTexImage->MarkReload();

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
	// 如果路径不合法，不显示纹理信息
	if (path.empty())
	{
		m_pTexInfo = nullptr;
		return;
	}

	if (m_pTexImage)
	{
		// 如果路径相同，无需重新加载
		if (path == m_pTexImage->GetFilePath())
		{
			m_pTexInfo = m_pTexImage->GetTextureNXInfo();
			return;
		}

		// 路径不同，释放旧的纹理，并且不显示纹理信息
		m_pTexImage->RemoveRef();
		m_pTexInfo = nullptr;
	}

	// 如果和之前的路径不同，就加载新的纹理
	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
	if (m_pTexImage)
		m_pTexInfo = m_pTexImage->GetTextureNXInfo();
}
