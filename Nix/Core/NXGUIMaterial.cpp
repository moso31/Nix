#include "NXGUIMaterial.h"
#include <unordered_set>
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXGUIContentExplorer.h"

const char* NXGUIMaterial::s_strCBufferGUIStyle[] = { "Value", "Value2", "Value3", "Value4", "Slider", "Slider2", "Slider3", "Slider4", "Color3", "Color4" };

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser),
	m_whiteTexPath_test(L".\\Resource\\white1x1.png"),
	m_normalTexPath_test(L".\\Resource\\normal1x1.png"),
	m_currentMaterialTypeIndex(0),
	m_pLastMaterial(nullptr)
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
				for (NXSubMeshBase* pSubMesh : pPickingSubMeshes) 
					pSubMesh->MarkReplacing(pDropData->srcPath);
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

		static const char* items[] = { "Standard", "Translucent", "Subsurface", "Custom"};
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
		case PBR_SUBSURFACE:
			RenderMaterialUI_Subsurface(static_cast<NXPBRMaterialSubsurface*>(pCommonMaterial));
			break;
		case CUSTOM:
			RenderMaterialUI_Custom(static_cast<NXCustomMaterial*>(pCommonMaterial));
			break;
		default:
			break;
		}

		if (matType != newMatType)
			NXResourceManager::GetInstance()->GetMaterialManager()->ReTypeMaterial(pCommonMaterial, newMatType);

		// 保存当前材质
		if (ImGui::Button("Save##material"))
		{
			pCommonMaterial->GetFilePath();

			NXGUICommon::SaveMaterialFile(pCommonMaterial);
		}

		if (pCommonMaterial->IsCustomMat())
		{
			if (ImGui::Button("Compile##Material_Compile"))
			{
				OnBtnCompileClicked(static_cast<NXCustomMaterial*>(pCommonMaterial));
			}
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

void NXGUIMaterial::OnBtnAddParamClicked(NXCustomMaterial* pMaterial)
{
	//m_customParamInfos.push_back({ "gg", NXCBufferInputType::Float, NXGUICustomMatParamStyle::eValue });
}

void NXGUIMaterial::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
}

void NXGUIMaterial::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

void NXGUIMaterial::SyncMaterialData(NXCustomMaterial* pMaterial)
{
	m_cbInfosDisplay.clear();
	m_cbInfosDisplay.reserve(pMaterial->GetCBufferElemCount());
	for (UINT i = 0; i < pMaterial->GetCBufferElemCount(); i++)
	{
		auto& cbElem = pMaterial->GetCBufferElem(i);
		const float* cbElemData = pMaterial->GetCBInfoMemoryData(cbElem.memoryIndex);

		Vector4 cbDataDisplay(cbElemData);
		switch (cbElem.type)
		{
		case NXCBufferInputType::Float: cbDataDisplay = { cbDataDisplay.x, 0.0f, 0.0f, 0.0f }; break;
		case NXCBufferInputType::Float2: cbDataDisplay = { cbDataDisplay.x, cbDataDisplay.y, 0.0f, 0.0f }; break;
		case NXCBufferInputType::Float3: cbDataDisplay = { cbDataDisplay.x, cbDataDisplay.y, cbDataDisplay.z, 0.0f }; break;
		default: break;
		}

		// 将cbElem.type 强转成 NXGUICBufferStyle，对应 GUIStyle 中的 Value/2/3/4
		// 换句话说 Float/2/3/4 默认使用 Value/2/3/4 GUI Style。
		NXGUICBufferStyle guiStyle = GetDefaultGUIStyleFromCBufferType(cbElem.type);

		// 根据 GUI Style 设置GUI的拖动速度或最大最小值
		Vector2 guiParams = GetGUIParamsDefaultValue(guiStyle);

		m_cbInfosDisplay.push_back({ cbElem.name, cbDataDisplay, guiStyle, guiParams, cbElem.memoryIndex });
	}

	m_pLastMaterial = pMaterial;
}

NXGUICBufferStyle NXGUIMaterial::GetGUIStyleFromString(const std::string& strTypeString)
{
	if (strTypeString == "Value") return NXGUICBufferStyle::Value;
	else if (strTypeString == "Value2") return NXGUICBufferStyle::Value2;
	else if (strTypeString == "Value3") return NXGUICBufferStyle::Value3;
	else if (strTypeString == "Value4") return NXGUICBufferStyle::Value4;
	else if (strTypeString == "Slider") return NXGUICBufferStyle::Slider;
	else if (strTypeString == "Slider2") return NXGUICBufferStyle::Slider2;
	else if (strTypeString == "Slider3") return NXGUICBufferStyle::Slider3;
	else if (strTypeString == "Slider4") return NXGUICBufferStyle::Slider4;
	else if (strTypeString == "Color3") return NXGUICBufferStyle::Color3;
	else if (strTypeString == "Color4") return NXGUICBufferStyle::Color4;
	else throw std::runtime_error("Invalid GUI style string: " + strTypeString);
}

NXGUICBufferStyle NXGUIMaterial::GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType)
{
	switch (eCBElemType)
	{
	case NXCBufferInputType::Float: 
		return NXGUICBufferStyle::Value;
	case NXCBufferInputType::Float2: 
		return NXGUICBufferStyle::Value2;
	case NXCBufferInputType::Float3: 
		return NXGUICBufferStyle::Value3;
	case NXCBufferInputType::Float4: 
	default:
		return NXGUICBufferStyle::Value4;
	}
}

Vector2 NXGUIMaterial::GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle)
{
	switch (eGUIStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		return { 0.01f, 0.0f }; // speed, ---

	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
	case NXGUICBufferStyle::Color3:
	case NXGUICBufferStyle::Color4:
		return { 0.0f, 1.0f }; // min, max
	}

	return { 0.01f, 0.0f }; // speed, ---
}

UINT NXGUIMaterial::GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle)
{
	switch (eGuiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Slider:
		return 1;
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Slider2:
		return 2;
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Color3:
		return 3;
	case NXGUICBufferStyle::Value4:
	case NXGUICBufferStyle::Slider4:
	case NXGUICBufferStyle::Color4:
	default:
		return 4;
	}
}

void NXGUIMaterial::RenderMaterialUI_Standard(NXPBRMaterialStandard* pMaterial)
{
	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 albedo = pMaterial->GetAlbedo();
	float vAlbedo[3] = { albedo.x, albedo.y, albedo.z };
	if (ImGui::ColorEdit3("Albedo", vAlbedo))
	{
		pMaterial->SetAlbedo(Vector3(vAlbedo));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = {normal.x, normal.y, normal.z};
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Translucent(NXPBRMaterialTranslucent* pMaterial)
{
	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();

	Vector3 albedo = pMaterial->GetAlbedo();
	float opacity = pMaterial->GetOpacity();
	float v[4] = { albedo.x, albedo.y, albedo.z, opacity };
	if (ImGui::ColorEdit4("Albedo", v))
	{
		pMaterial->SetAlbedo(Vector3(v));
		pMaterial->SetOpacity(v[3]);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = { normal.x, normal.y, normal.z };
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Subsurface(NXPBRMaterialSubsurface* pMaterial)
{
	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();

	Vector3 albedo = pMaterial->GetAlbedo();
	float opacity = pMaterial->GetOpacity();
	float v[4] = { albedo.x, albedo.y, albedo.z, opacity };
	if (ImGui::ColorEdit4("Albedo", v))
	{
		pMaterial->SetAlbedo(Vector3(v));
		pMaterial->SetOpacity(v[3]);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = { normal.x, normal.y, normal.z };
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom(NXCustomMaterial* pMaterial)
{
	if (m_pLastMaterial != pMaterial)
	{
		SyncMaterialData(pMaterial);
	}

	ImGui::BeginChild("##material_custom", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f));
	{
		// 禁用树节点首行缩进
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		RenderMaterialUI_Custom_Parameters(pMaterial);
		RenderMaterialUI_Custom_Codes(pMaterial);
		ImGui::PopStyleVar(); // ImGuiStyleVar_IndentSpacing
		ImGui::EndChild();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial)
{
	if (ImGui::TreeNode("Parameters##material_custom_parameters"))
	{
		if (ImGui::Button("Add param##material_custom_parameters_add", ImVec2(ImGui::GetContentRegionAvail().x, 20.0f)))
		{
			// 添加参数
			OnBtnAddParamClicked(pMaterial);
		}

		ImGui::BeginChild("##material_custom_child");
		{
			int paramCnt = 0;
			for (auto& cbDisplay : m_cbInfosDisplay)
			{
				std::string labelTypeId = "##material_custom_child_combo_type_" + std::to_string(paramCnt++);
				if (ImGui::BeginCombo(labelTypeId.c_str(), s_strCBufferGUIStyle[(int)cbDisplay.guiStyle]))
				{
					for (int item = 0; item < IM_ARRAYSIZE(s_strCBufferGUIStyle); item++)
					{
						if (ImGui::Selectable(s_strCBufferGUIStyle[item]) && item != (int)cbDisplay.guiStyle)
						{
							// 设置 GUI Style
							cbDisplay.guiStyle = GetGUIStyleFromString(s_strCBufferGUIStyle[item]);

							// 根据 GUI Style 设置GUI的拖动速度或最大最小值
							cbDisplay.params = GetGUIParamsDefaultValue(cbDisplay.guiStyle);
							break;
						}
					}
					ImGui::EndCombo();
				}

				RenderMaterialUI_Custom_Parameters_CBufferItem(pMaterial, cbDisplay);
			}
			ImGui::EndChild();
		}
		ImGui::TreePop();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters_CBufferItem(NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay)
{
	bool bDraged = false;

	UINT N = GetValueNumOfGUIStyle(cbDisplay.guiStyle);
	switch (cbDisplay.guiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		bDraged |= ImGui::DragScalarN(cbDisplay.name.data(), ImGuiDataType_Float, cbDisplay.data, N, cbDisplay.params[0]);
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		bDraged |= ImGui::SliderScalarN(cbDisplay.name.data(), ImGuiDataType_Float, cbDisplay.data, N, &cbDisplay.params[0], &cbDisplay.params[1]);
		break;
	case NXGUICBufferStyle::Color3:
		bDraged |= ImGui::ColorEdit3(cbDisplay.name.data(), cbDisplay.data);
		break;
	case NXGUICBufferStyle::Color4:
		bDraged |= ImGui::ColorEdit4(cbDisplay.name.data(), cbDisplay.data);
		break;
	}

	// 上面所有GUI的拖动事件都可以表示成下面的方法
	if (bDraged)
	{
		pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, N, cbDisplay.data);
		pMaterial->UpdateCBData();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Codes(NXCustomMaterial* pMaterial)
{
	if (ImGui::TreeNode("Codes"))
	{
		auto strCode = pMaterial->GetNSLCode();
		static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
		ImGui::InputTextMultiline("##material_custom_paramview_text", &strCode, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);
		ImGui::TreePop();
	}
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
