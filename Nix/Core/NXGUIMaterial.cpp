#include "NXGUIMaterial.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser),
	m_bMaterialDirty(false)
{
}

void NXGUIMaterial::Render()
{
	m_bMaterialDirty = false;

	NXPrimitive* pPickingObject = static_cast<NXPrimitive*>(m_pCurrentScene->GetCurrentPickingObject());
	if (!pPickingObject)
		return;

	ImGui::Begin("Material");

	std::string strName = pPickingObject->GetName().c_str();
	ImGui::InputText("Name", &strName);

	float fDrugSpeedTransform = 0.001f;
	XMVECTOR vTrans = XMLoadFloat3(&pPickingObject->GetTranslation());
	if (ImGui::DragFloat3("Translation", vTrans.m128_f32, fDrugSpeedTransform))
	{
		pPickingObject->SetTranslation(vTrans);
	}

	XMVECTOR vRot = XMLoadFloat3(&pPickingObject->GetRotation().EulerXYZ());
	if (ImGui::DragFloat3("Rotation", vRot.m128_f32, fDrugSpeedTransform))
	{
		Quaternion qRot(vRot);
		pPickingObject->SetRotation(Quaternion(qRot));
	}

	XMVECTOR vScal = XMLoadFloat3(&pPickingObject->GetScale());
	if (ImGui::DragFloat3("Scale", vScal.m128_f32, fDrugSpeedTransform))
	{
		pPickingObject->SetScale(vScal);
	}

	NXPBRMaterial* pPickingObjectMaterial = pPickingObject->GetPBRMaterial();
	if (pPickingObjectMaterial)
	{
		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pPickingObjectMaterial));
		ImGui::SameLine();
		XMVECTORF32 fAlbedo;
		fAlbedo.v = pPickingObjectMaterial->m_albedo;
		if (ImGui::ColorEdit3("Albedo", fAlbedo.f))
		{
			pPickingObjectMaterial->m_albedo = fAlbedo.v;
			m_bMaterialDirty = true;
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pPickingObjectMaterial));
		ImGui::SameLine();
		XMVECTORF32 fNormal;
		fNormal.v = pPickingObjectMaterial->m_normal;
		if (ImGui::ColorEdit3("Normal", fNormal.f))
		{
			pPickingObjectMaterial->m_normal = fNormal.v;
			m_bMaterialDirty = true;
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pPickingObjectMaterial));
		ImGui::SameLine();
		if (ImGui::SliderFloat("Metallic", &pPickingObjectMaterial->m_metallic, 0.0f, 1.0f))
		{
			m_bMaterialDirty = true;
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pPickingObjectMaterial));
		ImGui::SameLine();
		if (ImGui::SliderFloat("Roughness", &pPickingObjectMaterial->m_roughness, 0.0f, 1.0f))
		{
			m_bMaterialDirty = true;
		}

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pPickingObjectMaterial));
		ImGui::SameLine();
		if (ImGui::SliderFloat("AO", &pPickingObjectMaterial->m_ao, 0.0f, 1.0f))
		{
			m_bMaterialDirty = true;
		}
	}

	ImGui::End();

	if (m_bMaterialDirty)
		UpdateMaterial(pPickingObjectMaterial);
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
	pPickingObjectMaterial->SetTexAO(m_pFileBrowser->GetSelected().c_str());
}

void NXGUIMaterial::RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange)
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
			m_pFileBrowser->Open();
			m_pFileBrowser->SetOnDialogOK(onChange);
		}

		ImGui::SameLine();
		ImGui::PushID("RemoveTexButtons");
		{
			ImGui::PushID(ImTexID);
			if (ImGui::Button("R"))
			{
			}
			ImGui::PopID();
		}
		ImGui::PopID();
	}
}

void NXGUIMaterial::UpdateMaterial(NXPBRMaterial* pMaterial)
{
	if (m_bMaterialDirty)
	{
		auto pRefPrims = pMaterial->GetPrimitives();
		for (auto pPrim : pRefPrims)
		{
			pPrim->SetMaterialPBR(pMaterial);
		}
	}
}
