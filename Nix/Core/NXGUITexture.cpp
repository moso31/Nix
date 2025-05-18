#include "BaseDefs/DearImGui.h"

#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"
#include "NXGUIInspector.h"
#include "NXAllocatorManager.h"

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

	if (m_path.extension() == ".raw")
	{
		Render_RawTexture();
	}
	else
	{
		Render_Texture();
	}
}

void NXGUITexture::Release()
{
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	if (m_pTexImage.IsValid() && path == m_pTexImage->GetFilePath())
		return;

	m_path = path;
	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
	m_texData = m_pTexImage->GetSerializationData();
}

void NXGUITexture::Render_Texture()
{
	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	NXShVisDescHeap->PushFluid(m_pTexImage->GetSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	const ImTextureID& ImTexID = (ImTextureID)srvHandle.ptr;
	ImGui::Image(ImTexID, ImVec2(fTexSize, fTexSize));

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
		// 根据GUI参数更新序列化相关数据
		m_pTexImage->SetSerializationData(m_texData);

		// 序列化，保存成n0文件
		m_pTexImage->Serialize();

		// 进行异步重载
		m_pTexImage->MarkReload(m_pTexImage->GetFilePath());
	}
}

void NXGUITexture::Render_RawTexture()
{
	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	NXShVisDescHeap->PushFluid(m_pTexImage->GetSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	const ImTextureID& ImTexID = (ImTextureID)srvHandle.ptr;
	ImGui::Image(ImTexID, ImVec2(fTexSize, fTexSize));

	static int value[2] = { m_texData.m_rawWidth, m_texData.m_rawHeight };
	if (ImGui::InputInt2("Texture Size", value))
	{
		m_texData.m_rawWidth = value[0];
		m_texData.m_rawHeight = value[1];
	}

	if (ImGui::Button("Apply##Texture"))
	{
		// 根据GUI参数更新序列化相关数据
		m_pTexImage->SetSerializationData(m_texData);

		// 序列化，保存成n0文件
		m_pTexImage->Serialize();

		// 进行异步重载
		m_pTexImage->MarkReload(m_pTexImage->GetFilePath());
	}
}
