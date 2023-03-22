#include "NXGUIMaterial.h"
#include <unordered_set>
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "SceneManager.h"
#include "NXGUIContentExplorer.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser),
	m_whiteTexPath_test(L".\\Resource\\white1x1.png"),
	m_normalTexPath_test(L".\\Resource\\normal1x1.png"),
	m_currentMaterialTypeIndex(0)
{
}

void NXGUIMaterial::Render()
{
	ImGui::Begin("Material");
	std::vector<NXSubMeshBase*> pPickingSubMeshes = m_pCurrentScene->GetPickingSubMeshes();
	if (pPickingSubMeshes.empty())
	{
		ImGui::End();
		return;
	}

	// 统计选中的所有Meshes里面有多少材质
	std::unordered_set<NXMaterial*> pUniqueMats;
	for (auto pSubMesh : pPickingSubMeshes)
		pUniqueMats.insert(pSubMesh->GetMaterial());

	// 如果选中的所有SubMesh都只有一个材质，将此材质记作pCommonMaterial
	bool bIsReadOnlyMaterial = pUniqueMats.size() != 1;
	NXMaterial* pCommonMaterial = bIsReadOnlyMaterial ? nullptr : *pUniqueMats.begin();

	bool bIsReadOnlyTransform = pPickingSubMeshes.size() != 1;
	if (bIsReadOnlyTransform) ImGui::BeginDisabled();

	NXPrimitive* pObject = pPickingSubMeshes[0]->GetPrimitive();
	NXMaterial* pMaterial = pPickingSubMeshes[0]->GetMaterial();

	std::string strName = bIsReadOnlyTransform ? "-" : pObject->GetName().c_str();
	if (ImGui::InputText("Name", &strName))
	{
		pObject->SetName(strName);
	}

	float fDrugSpeedTransform = 0.01f;
	Vector3 vTrans = bIsReadOnlyTransform ? Vector3(0.0f) : pObject->GetTranslation();
	float vTransArr[3] = { vTrans.x, vTrans.y, vTrans.z };
	if (ImGui::DragFloat3("Translation", vTransArr, fDrugSpeedTransform))
	{
		pObject->SetTranslation(vTrans);
	}

	Vector3 vRot = bIsReadOnlyTransform ? Vector3(0.0f) : pObject->GetRotation();
	float vRotArr[3] = { vRot.x, vRot.y, vRot.z };
	if (ImGui::DragFloat3("Rotation", vRotArr, fDrugSpeedTransform))
	{
		pObject->SetRotation(Vector3(vRotArr));

		// 没什么意义的辣鸡测试……
		//{
		//	Vector3 value(0.2, 1.12, 2.31);
		//	Quaternion _qRot = Quaternion::CreateFromYawPitchRoll(value.y, value.x, value.z);
		//	Vector3 res = _qRot.EulerXYZ();
		//	printf("%f %f %f\n", res.x, res.y, res.z);
		//}
	}

	Vector3 vScal = bIsReadOnlyTransform ? Vector3(0.0f) : pObject->GetScale();
	float vScalArr[3] = { vScal.x, vScal.y, vScal.z };
	if (ImGui::DragFloat3("Scale", vScalArr, fDrugSpeedTransform))
	{
		pObject->SetScale(vScal);
	}

	if (bIsReadOnlyTransform) ImGui::EndDisabled();

	ImGui::Text("%d Materials, %d Submeshes", pUniqueMats.size(), pPickingSubMeshes.size());
	ImGui::Separator();

	float fBtnSize = 45.0f;
	ImGui::BeginChild("##material_iconbtn", ImVec2(fBtnSize, max(ImGui::GetContentRegionAvail().y * 0.1f, fBtnSize)));
	ImGui::Button(".nmat##iconbtn", ImVec2(fBtnSize, fBtnSize));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
		{
			auto pDropData = (NXGUIContentExplorerButtonDrugData*)(payload->Data);
			if (NXConvert::IsMaterialFileExtension(pDropData->srcPath.extension().string()))
			{
				// 生成新材质
				auto pNewMaterial = SceneManager::GetInstance()->LoadFromNmatFile(pDropData->srcPath);

				// 替换物体原来的材质
				for (auto pSubMesh : pPickingSubMeshes)
				{
					SceneManager::GetInstance()->BindMaterial(pSubMesh, pNewMaterial);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::EndChild();

	ImGui::SameLine();

	if (pCommonMaterial)
	{
		ImGui::BeginChild("##material_description", ImVec2(ImGui::GetContentRegionAvail().x, max(ImGui::GetContentRegionAvail().y * 0.1f, fBtnSize)));
		std::string strMatName = pCommonMaterial->GetName().c_str();
		if (ImGui::InputText("Material", &strMatName))
		{
			pCommonMaterial->SetName(strMatName);
		}

		NXMaterialType matType = pCommonMaterial->GetType();
		m_currentMaterialTypeIndex = matType - 1;

		static const char* items[] = { "Standard", "Translucent" };
		ImGui::Combo("Material Type", &m_currentMaterialTypeIndex, items, IM_ARRAYSIZE(items));
		ImGui::EndChild();

		NXMaterialType newMatType = NXMaterialType(m_currentMaterialTypeIndex + 1);
		switch (newMatType)
		{
		case UNKNOWN:
			break;
		case PBR_STANDARD:
			RenderMaterialUI_Standard(static_cast<NXPBRMaterialStandard*>(pCommonMaterial));
			break;
		case PBR_TRANSLUCENT:
			RenderMaterialUI_Translucent(static_cast<NXPBRMaterialTranslucent*>(pCommonMaterial));
			break;
		default:
			break;
		}

		if (matType != newMatType)
			SceneManager::GetInstance()->ReTypeMaterial(pCommonMaterial, newMatType);

		// 保存当前材质
		if (ImGui::Button("Save##material"))
		{
			pCommonMaterial->GetFilePath();

			NXGUICommon::SaveMaterialFile(pCommonMaterial);
		}
	}

	ImGui::End();
}

void NXGUIMaterial::OnTexAlbedoChange(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexAlbedo(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexNormalChange(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexNormal(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexMetallicChange(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexMetallic(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexRoughnessChange(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexRoughness(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexAOChange(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexAO(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexAlbedoRemove(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexAlbedo(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexNormalRemove(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexNormal(m_normalTexPath_test);
}

void NXGUIMaterial::OnTexMetallicRemove(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexMetallic(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexRoughnessRemove(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexRoughness(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexAORemove(NXPBRMaterialBase* pMaterial)
{
	pMaterial->SetTexAO(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexAlbedoDrop(NXPBRMaterialBase* pMaterial, const std::wstring& filePath)
{
	pMaterial->SetTexAlbedo(filePath.c_str());
}

void NXGUIMaterial::OnTexNormalDrop(NXPBRMaterialBase* pMaterial, const std::wstring& filePath)
{
	pMaterial->SetTexNormal(filePath.c_str());
}

void NXGUIMaterial::OnTexMetallicDrop(NXPBRMaterialBase* pMaterial, const std::wstring& filePath)
{
	pMaterial->SetTexMetallic(filePath.c_str());
}

void NXGUIMaterial::OnTexRoughnessDrop(NXPBRMaterialBase* pMaterial, const std::wstring& filePath)
{
	pMaterial->SetTexRoughness(filePath.c_str());
}

void NXGUIMaterial::OnTexAODrop(NXPBRMaterialBase* pMaterial, const std::wstring& filePath)
{
	pMaterial->SetTexAO(filePath.c_str());
}

void NXGUIMaterial::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

void NXGUIMaterial::RenderMaterialUI_Standard(NXPBRMaterialStandard* pMaterial)
{
	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	XMVECTORF32 fAlbedo;
	fAlbedo.v = pMaterial->GetAlbedo();
	if (ImGui::ColorEdit3("Albedo", fAlbedo.f))
	{
		pMaterial->SetAlbedo(fAlbedo.v);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	XMVECTORF32 fNormal;
	fNormal.v = pMaterial->GetNormal();
	if (ImGui::ColorEdit3("Normal", fNormal.f))
	{
		pMaterial->SetNormal(fNormal.v);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("Metallic", pMaterial->GetMetallic(), 0.0f, 1.0f);

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("Roughness", pMaterial->GetRoughness(), 0.0f, 1.0f);

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("AO", pMaterial->GetAO(), 0.0f, 1.0f);
}

void NXGUIMaterial::RenderMaterialUI_Translucent(NXPBRMaterialTranslucent* pMaterial)
{
	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	XMVECTORF32 fAlbedo;
	fAlbedo.v = pMaterial->GetAlbedo();
	fAlbedo.f[3] = *pMaterial->GetOpacity();
	if (ImGui::ColorEdit4("Albedo", fAlbedo.f))
	{
		pMaterial->SetAlbedo(fAlbedo.v);
		pMaterial->SetOpacity(fAlbedo.f[3]);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	XMVECTORF32 fNormal;
	fNormal.v = pMaterial->GetNormal();
	if (ImGui::ColorEdit3("Normal", fNormal.f))
	{
		pMaterial->SetNormal(fNormal.v);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("Metallic", pMaterial->GetMetallic(), 0.0f, 1.0f);

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("Roughness", pMaterial->GetRoughness(), 0.0f, 1.0f);

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	ImGui::SliderFloat("AO", pMaterial->GetAO(), 0.0f, 1.0f);
}

void NXGUIMaterial::RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop)
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
				if (NXConvert::IsImageFileExtension(pDropData->srcPath.extension().string()))
				{
					onDrop(pDropData->srcPath.wstring());
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();
		ImGui::PushID("RemoveTexButtons");
		{
			ImGui::PushID(ImTexID);
			if (ImGui::Button("R"))
			{
				onRemove();
			}
			ImGui::PopID();
		}
		ImGui::PopID();
	}
}
