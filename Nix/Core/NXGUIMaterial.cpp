#include "NXGUIMaterial.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser),
	m_whiteTexPath_test(L".\\Resource\\white1x1.png"),
	m_normalTexPath_test(L".\\Resource\\normal1x1.png")
{
}

void NXGUIMaterial::Render()
{
	ImGui::Begin("Material");
	NXSubMesh* pPickingSubMesh = m_pCurrentScene->GetCurrentPickingSubMesh();
	if (!pPickingSubMesh)
	{
		ImGui::End();
		return;
	}

	NXPrimitive* pPickingObject = pPickingSubMesh->GetPrimitive();
	NXPBRMaterial* pPickingObjectMaterial = pPickingSubMesh->GetPBRMaterial();

	std::string strName = pPickingObject->GetName().c_str();
	if (ImGui::InputText("Name", &strName))
	{
		pPickingObject->SetName(strName);
	}

	std::string strMatName = pPickingObjectMaterial->GetName().c_str();
	if (ImGui::InputText("Material", &strMatName))
	{
		pPickingObjectMaterial->SetName(strMatName);
	}

	float fDrugSpeedTransform = 0.01f;
	XMVECTOR vTrans = XMLoadFloat3(&pPickingObject->GetTranslation());
	if (ImGui::DragFloat3("Translation", vTrans.m128_f32, fDrugSpeedTransform))
	{
		pPickingObject->SetTranslation(vTrans);
	}

	Quaternion qRot = pPickingObject->GetRotation();
	Vector3 vRot = qRot.EulerXYZ();
	float vRotArr[3] = { vRot.x, vRot.y, vRot.z };
	if (ImGui::DragFloat3("Rotation", vRotArr, fDrugSpeedTransform))
	{
		pPickingObject->SetRotation(Vector3(vRotArr));

		// 没什么意义的辣鸡测试……
		//{
		//	Vector3 value(0.2, 1.12, 2.31);
		//	Quaternion _qRot = Quaternion::CreateFromYawPitchRoll(value.y, value.x, value.z);
		//	Vector3 res = _qRot.EulerXYZ();
		//	printf("%f %f %f\n", res.x, res.y, res.z);
		//}
	}

	XMVECTOR vScal = XMLoadFloat3(&pPickingObject->GetScale());
	if (ImGui::DragFloat3("Scale", vScal.m128_f32, fDrugSpeedTransform))
	{
		pPickingObject->SetScale(vScal);
	}

	if (pPickingObjectMaterial)
	{
		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pPickingObjectMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pPickingObjectMaterial));
		ImGui::SameLine();
		XMVECTORF32 fAlbedo;
		fAlbedo.v = pPickingObjectMaterial->GetAlbedo();
		if (ImGui::ColorEdit3("Albedo", fAlbedo.f))
		{
			pPickingObjectMaterial->SetAlbedo(fAlbedo.v);
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pPickingObjectMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pPickingObjectMaterial));
		ImGui::SameLine();
		XMVECTORF32 fNormal;
		fNormal.v = pPickingObjectMaterial->GetNormal();
		if (ImGui::ColorEdit3("Normal", fNormal.f))
		{
			pPickingObjectMaterial->SetNormal(fNormal.v);
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pPickingObjectMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pPickingObjectMaterial));
		ImGui::SameLine();
		ImGui::SliderFloat("Metallic", pPickingObjectMaterial->GetMatallic(), 0.0f, 1.0f);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pPickingObjectMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pPickingObjectMaterial));
		ImGui::SameLine();
		ImGui::SliderFloat("Roughness", pPickingObjectMaterial->GetRoughness(), 0.0f, 1.0f);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pPickingObjectMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pPickingObjectMaterial));
		ImGui::SameLine();
		ImGui::SliderFloat("AO", pPickingObjectMaterial->GetAO(), 0.0f, 1.0f);
	}

	ImGui::End();
}

void NXGUIMaterial::OnTexAlbedoChange(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexAlbedo(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexNormalChange(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexNormal(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexMetallicChange(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexMetallic(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexRoughnessChange(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexRoughness(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::OnTexAOChange(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexAO(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexAlbedoRemove(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexAlbedo(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexNormalRemove(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexNormal(m_normalTexPath_test);
}

void NXGUIMaterial::OnTexMetallicRemove(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexMetallic(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexRoughnessRemove(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexRoughness(m_whiteTexPath_test);
}

void NXGUIMaterial::OnTexAORemove(NXPBRMaterial* pPickingObjectMaterial)
{
	pPickingObjectMaterial->SetTexAO(m_whiteTexPath_test);
}

void NXGUIMaterial::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

void NXGUIMaterial::RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove)
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
