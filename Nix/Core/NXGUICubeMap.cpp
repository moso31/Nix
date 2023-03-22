#include "NXGUICubeMap.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXResourceReloader.h"
#include "NXGUIContentExplorer.h"

NXGUICubeMap::NXGUICubeMap(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser)
{
}

void NXGUICubeMap::Render()
{
	NXCubeMap* pCubeMap = m_pCurrentScene->GetCubeMap();

	ImGui::Begin("CubeMap");

	RenderTextureIcon((ImTextureID)pCubeMap->GetSRVCubeMapPreview2D(), std::bind(&NXGUICubeMap::OnCubeMapTexChange, this, pCubeMap), std::bind(&NXGUICubeMap::OnCubeMapTexDrop, this, pCubeMap, std::placeholders::_1));

	ImGui::SliderFloat("Intensity", pCubeMap->GetIntensity(), 0.0f, 10.0f);

	static int x = 0;
	static const char* items[] = { "Cube Map", "Irradiance Map"};
	ImGui::Combo("Material Type", &x, items, IM_ARRAYSIZE(items));
	pCubeMap->SetIrradMode(x);


	ImGui::End();
}

void NXGUICubeMap::RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void(const std::wstring&)> onDrop)
{
	float my_tex_w = (float)16;
	float my_tex_h = (float)16;

	ImGuiIO& io = ImGui::GetIO();
	{
		//ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
		//ImVec2 pos = ImGui::GetCursorScreenPos();
		//ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		//ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		//ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		//ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
		//ImGui::Image(ImTexID, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
		//if (ImGui::IsItemHovered())
		//{
		//	ImGui::BeginTooltip();
		//	float region_sz = 32.0f;
		//	float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
		//	float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
		//	float zoom = 4.0f;
		//	if (region_x < 0.0f) { region_x = 0.0f; }
		//	else if (region_x > my_tex_w - region_sz) { region_x = my_tex_w - region_sz; }
		//	if (region_y < 0.0f) { region_y = 0.0f; }
		//	else if (region_y > my_tex_h - region_sz) { region_y = my_tex_h - region_sz; }
		//	ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
		//	ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
		//	ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
		//	ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
		//	ImGui::Image(ImTexID, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
		//	ImGui::EndTooltip();
		//}

		int frame_padding = 2;									// -1 == uses default padding (style.FramePadding)
		ImVec2 size = ImVec2(16.0f, 16.0f);                     // Size of the image we want to make visible
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);         // Black background
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);       // No tint
		if (ImGui::ImageButton(ImTexID, size, uv0, uv1, frame_padding, bg_col, tint_col))
		{
			UpdateFileBrowserParameters();
			m_pFileBrowser->Open();
			m_pFileBrowser->SetOnDialogOK(onChange);
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
			{
				auto pDropData = (NXGUIContentExplorerButtonDrugData*)(payload->Data);
				if (DropDataIsCubeMapImage(pDropData))
				{
					onDrop(pDropData->srcPath.wstring());
				}
			}
			ImGui::EndDragDropTarget();
		}

		//ImGui::SameLine();
		//ImGui::PushID("RemoveTexButtons");
		//{
		//	ImGui::PushID(ImTexID);
		//	if (ImGui::Button("R"))
		//	{
		//		onRemove();
		//	}
		//	ImGui::PopID();
		//}
		//ImGui::PopID();
	}
}

void NXGUICubeMap::OnCubeMapTexChange(NXCubeMap* pCubeMap)
{
	NXResourceReloadCubeMapCommand* pCommand = new NXResourceReloadCubeMapCommand();
	pCommand->pCubeMap = pCubeMap;
	pCommand->strFilePath = m_pFileBrowser->GetSelected().c_str();
	NXResourceReloader::GetInstance()->Push(pCommand);
}

void NXGUICubeMap::OnCubeMapTexDrop(NXCubeMap* pCubeMap, const std::wstring& filePath)
{
	NXResourceReloadCubeMapCommand* pCommand = new NXResourceReloadCubeMapCommand();
	pCommand->pCubeMap = pCubeMap;
	pCommand->strFilePath = filePath.c_str();
	NXResourceReloader::GetInstance()->Push(pCommand);
}

void NXGUICubeMap::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".*", ".hdr" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

bool NXGUICubeMap::DropDataIsCubeMapImage(NXGUIContentExplorerButtonDrugData* pDropData)
{
	std::string strExtension = pDropData->srcPath.extension().string();
	std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), [](UCHAR c) { return std::tolower(c); });

	return strExtension == ".hdr" || strExtension == ".dds";
}
