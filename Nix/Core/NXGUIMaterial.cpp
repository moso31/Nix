#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXGUIContentExplorer.h"
#include "NXGUIMaterialShaderEditor.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser),
	m_whiteTexPath_test(L".\\Resource\\white1x1.png"),
	m_normalTexPath_test(L".\\Resource\\normal1x1.png"),
	m_currentMaterialTypeIndex(0),
	m_pLastMaterial(nullptr),
	m_bIsDirty(false)
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
			auto pDropData = (NXGUIAssetDragData*)(payload->Data);
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
			ImGui::SameLine();
			if (ImGui::Button("Edit Shader...##material_custom_editshader"))
			{
				OnBtnEditShaderClicked(static_cast<NXCustomMaterial*>(pCommonMaterial));
			}
		}
	}

	ImGui::End();

	// 渲染 Shader Editor GUI
	if (pCommonMaterial && pCommonMaterial->IsCustomMat())
	{
		GetShaderEditor()->Render(static_cast<NXCustomMaterial*>(pCommonMaterial));
	}
}

void NXGUIMaterial::Release()
{
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

void NXGUIMaterial::OnBtnEditShaderClicked(NXCustomMaterial* pMaterial)
{
	GetShaderEditor()->SetGUIMaterial(this);
	GetShaderEditor()->SetGUIFileBrowser(m_pFileBrowser);

	// 将参数和nsl代码从 当前GUI材质类 中同步到 ShaderEditor
	GetShaderEditor()->PrepareShaderResourceData(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);
	GetShaderEditor()->PrepareNSLCode(m_nslCodeDisplay);

	GetShaderEditor()->Show();
}

void NXGUIMaterial::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDataDisplay)
{
	using namespace NXGUICommon;

	// 设置 GUI Style
	cbDataDisplay.guiStyle = GetGUIStyleFromString(g_strCBufferGUIStyle[selectIndex]);

	// 根据 GUI Style 设置GUI的拖动速度或最大最小值
	cbDataDisplay.params = GetGUIParamsDefaultValue(cbDataDisplay.guiStyle);
}

void NXGUIMaterial::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

void NXGUIMaterial::SyncMaterialData(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

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

		// 如果之前 ShaderEditor 中有数据，并且能和 NXGUIMaterial 的材质名称对应上，就保留 ShaderEditor 的 GUIStyle
		NXGUICBufferStyle guiStyle;
		if (!GetShaderEditor()->FindCBStyle(cbElem.name, guiStyle))
		{
			// 如果 ShaderEditor 中没有对应的值，就读取材质原数据，根据 cbElem.type 的值生成对应的 NXGUICBufferStyle
			guiStyle = GetDefaultGUIStyleFromCBufferType(cbElem.type);
		}

		// 根据 GUI Style 设置GUI的拖动速度或最大最小值
		Vector2 guiParams = GetGUIParamsDefaultValue(guiStyle);

		m_cbInfosDisplay.push_back({ cbElem.name, cbElem.type, cbDataDisplay, guiStyle, guiParams, cbElem.memoryIndex });
	}

	m_texInfosDisplay.clear();
	m_texInfosDisplay.reserve(pMaterial->GetTextureCount());
	for (UINT i = 0; i < pMaterial->GetTextureCount(); i++)
		m_texInfosDisplay.push_back({ pMaterial->GetTextureName(i), pMaterial->GetTexture(i) });

	m_ssInfosDisplay.clear();
	m_ssInfosDisplay.reserve(pMaterial->GetSamplerCount());
	for (UINT i = 0; i < pMaterial->GetSamplerCount(); i++)
		m_ssInfosDisplay.push_back({ pMaterial->GetSamplerName(i), pMaterial->GetSampler(i) });

	m_nslCodeDisplay = pMaterial->GetNSLCode();

	m_pLastMaterial = pMaterial;
	m_bIsDirty = false;
}

NXGUIMaterialShaderEditor* NXGUIMaterial::GetShaderEditor()
{
	return NXGUIMaterialShaderEditor::GetInstance();
}

void NXGUIMaterial::RenderMaterialUI_Standard(NXPBRMaterialStandard* pMaterial)
{
	using namespace NXGUICommon;

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 albedo = pMaterial->GetAlbedo();
	float vAlbedo[3] = { albedo.x, albedo.y, albedo.z };
	if (ImGui::ColorEdit3("Albedo", vAlbedo))
	{
		pMaterial->SetAlbedo(Vector3(vAlbedo));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = {normal.x, normal.y, normal.z};
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Translucent(NXPBRMaterialTranslucent* pMaterial)
{
	using namespace NXGUICommon;

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();

	Vector3 albedo = pMaterial->GetAlbedo();
	float opacity = pMaterial->GetOpacity();
	float v[4] = { albedo.x, albedo.y, albedo.z, opacity };
	if (ImGui::ColorEdit4("Albedo", v))
	{
		pMaterial->SetAlbedo(Vector3(v));
		pMaterial->SetOpacity(v[3]);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = { normal.x, normal.y, normal.z };
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Subsurface(NXPBRMaterialSubsurface* pMaterial)
{
	using namespace NXGUICommon;

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAlbedo(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAlbedoChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAlbedoDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();

	Vector3 albedo = pMaterial->GetAlbedo();
	float opacity = pMaterial->GetOpacity();
	float v[4] = { albedo.x, albedo.y, albedo.z, opacity };
	if (ImGui::ColorEdit4("Albedo", v))
	{
		pMaterial->SetAlbedo(Vector3(v));
		pMaterial->SetOpacity(v[3]);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVNormal(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexNormalChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexNormalDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	Vector3 normal = pMaterial->GetNormal();
	float vNormal[3] = { normal.x, normal.y, normal.z };
	if (ImGui::ColorEdit3("Normal", vNormal))
	{
		pMaterial->SetNormal(Vector3(vNormal));
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVMetallic(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexMetallicChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexMetallicDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float metallic = pMaterial->GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
	{
		pMaterial->SetMetallic(metallic);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVRoughness(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexRoughnessChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessRemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexRoughnessDrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float roughness = pMaterial->GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
	{
		pMaterial->SetRoughness(roughness);
	}

	RenderTextureIcon((ImTextureID)pMaterial->GetSRVAO(), m_pFileBrowser, std::bind(&NXGUIMaterial::OnTexAOChange, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAORemove, this, pMaterial), std::bind(&NXGUIMaterial::OnTexAODrop, this, pMaterial, std::placeholders::_1));
	ImGui::SameLine();
	float ao = pMaterial->GetAO();
	if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
	{
		pMaterial->SetAO(ao);
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom(NXCustomMaterial* pMaterial)
{
	// TODO: 感觉 m_pLastMaterial 写在这里不太合适，写在类似 Update 之类的地方会更好
	// 但现在懒得做 GUI Update 的实现 所以先这么放着了……
	if (m_pLastMaterial != pMaterial)
		m_bIsDirty = true;

	// 将材质数据同步到 GUI材质类 和 ShaderEditor
	if (m_bIsDirty)
	{
		SyncMaterialData(pMaterial);
		GetShaderEditor()->PrepareShaderResourceData(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);
	}

	//ImGui::BeginChild("##material_custom", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f));
	{
		// 禁用树节点首行缩进
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		RenderMaterialUI_Custom_Parameters(pMaterial);
		RenderMaterialUI_Custom_Codes(pMaterial);
		ImGui::PopStyleVar(); // ImGuiStyleVar_IndentSpacing
		//ImGui::EndChild();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	if (ImGui::TreeNodeEx("Parameters##material_custom_parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("##material_custom_child");
		{
			int paramCnt = 0;
			for (auto& cbDisplay : m_cbInfosDisplay)
			{
				std::string strId = "##material_custom_child_cbuffer_" + std::to_string(paramCnt++);
				RenderMaterialUI_Custom_Parameters_CBufferItem(strId, pMaterial, cbDisplay);
			}

			for (auto& texDisplay : m_texInfosDisplay)
			{
				std::string strId = "##material_custom_child_texture_" + std::to_string(paramCnt);

				auto pTex = texDisplay.pTexture;
				if (!pTex) continue;

				auto onTexChange = [pMaterial, &pTex, this]()
				{
					pMaterial->SetTex2D(pTex, m_pFileBrowser->GetSelected().c_str());
				};

				auto onTexRemove = [pMaterial, &pTex, this]()
				{
					pMaterial->SetTex2D(pTex, m_pFileBrowser->GetSelected().c_str());
				};

				auto onTexDrop = [pMaterial, &pTex, this](const std::wstring& dragPath)
				{
					pMaterial->SetTex2D(pTex, dragPath);
				};

				ImGui::PushID(paramCnt);
				RenderTextureIcon(pTex->GetSRV(), m_pFileBrowser, onTexChange, onTexRemove, onTexDrop);
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::Text(texDisplay.name.data());

				paramCnt++;
			}

			// 【Sampler 的部分暂时还没想好，先空着】
			for (auto& ssDisplay : m_ssInfosDisplay)
			{
				std::string strId = "##material_custom_child_sampler_" + std::to_string(paramCnt++);
				ImGui::Text(ssDisplay.name.data());
			}

			ImGui::EndChild();
		}
		ImGui::TreePop();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay)
{
	using namespace NXGUICommon;

	bool bDraged = false;
	std::string strName = cbDisplay.name + strId + "_cb";

	UINT N = GetValueNumOfGUIStyle(cbDisplay.guiStyle);
	switch (cbDisplay.guiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		bDraged |= ImGui::DragScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, cbDisplay.params[0]);
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		bDraged |= ImGui::SliderScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, &cbDisplay.params[0], &cbDisplay.params[1]);
		break;
	case NXGUICBufferStyle::Color3:
		bDraged |= ImGui::ColorEdit3(strName.data(), cbDisplay.data);
		break;
	case NXGUICBufferStyle::Color4:
		bDraged |= ImGui::ColorEdit4(strName.data(), cbDisplay.data);
		break;
	}

	if (bDraged && cbDisplay.memoryIndex != -1) // 新加的 AddParam 在点编译按钮之前不应该传给参数
	{
		// 在这里将 GUI 修改过的参数传回给材质 CBuffer，实现视觉上的变化。
		// 实际上要拷贝的字节量是 cbDisplay 初始读取的字节数量 actualN，而不是更改 GUIStyle 以后的参数数量 N
		UINT actualN = cbDisplay.readType;
		pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, actualN, cbDisplay.data);
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Codes(NXCustomMaterial* pMaterial)
{
	if (ImGui::TreeNodeEx("Codes##material_custom_codes", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly;
		ImGui::InputTextMultiline("##material_custom_paramview_text", &m_nslCodeDisplay, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);
		ImGui::TreePop();
	}
}
