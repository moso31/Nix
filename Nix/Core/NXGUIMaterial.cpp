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

	// ͳ��ѡ�е�����Meshes�����ж��ٲ���
	std::unordered_set<NXMaterial*> pUniqueMats;
	for (auto pSubMesh : pPickingSubMeshes)
		pUniqueMats.insert(pSubMesh->GetMaterial());

	// ���ѡ�е�����SubMesh��ֻ��һ�����ʣ����˲��ʼ���pCommonMaterial
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

		// ûʲô������������ԡ���
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
		ImGui::EndChild();

		auto pCustomMat = pCommonMaterial->IsCustomMat();
		if (pCustomMat)
		{
			RenderMaterialUI_Custom(pCustomMat);

			// ���浱ǰ����
			if (ImGui::Button("Save##material"))
			{
				SaveMaterialFile(pCustomMat);
				pCommonMaterial->Serialize();
			}

			if (pCommonMaterial->IsCustomMat())
			{
				ImGui::SameLine();
				if (ImGui::Button("Edit Shader...##material_custom_editshader"))
				{
					OnBtnEditShaderClicked(pCustomMat);
				}
			}
		}
	}

	ImGui::End();

	// ��Ⱦ Shader Editor GUI
	if (pCommonMaterial && pCommonMaterial->IsCustomMat())
	{
		GetShaderEditor()->Render(static_cast<NXCustomMaterial*>(pCommonMaterial));
	}
}

void NXGUIMaterial::Release()
{
}

void NXGUIMaterial::SaveMaterialFile(NXCustomMaterial* pMaterial)
{
	pMaterial->SaveToNSLFile();
}

void NXGUIMaterial::OnBtnEditShaderClicked(NXCustomMaterial* pMaterial)
{
	GetShaderEditor()->SetGUIMaterial(this);
	GetShaderEditor()->SetGUIFileBrowser(m_pFileBrowser);

	// ��������nsl����� ��ǰGUI������ ��ͬ���� ShaderEditor
	GetShaderEditor()->RequestSyncMaterialData();

	GetShaderEditor()->Show();
}

void NXGUIMaterial::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDataDisplay)
{
	using namespace NXGUICommon;

	// ���� GUI Style
	cbDataDisplay.guiStyle = GetGUIStyleFromString(g_strCBufferGUIStyle[selectIndex]);

	// ���� GUI Style ����GUI���϶��ٶȻ������Сֵ
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

		// ���cb�д��� GUIStyle������ʹ�� GUIStyle ��ʾ cb
		NXGUICBufferStyle guiStyle = pMaterial->GetCBGUIStyles(i);
		if (guiStyle == NXGUICBufferStyle::Unknown)
		{
			// ������� cbElem �������Զ����� GUIStyle
			guiStyle = GetDefaultGUIStyleFromCBufferType(cbElem.type);
		}

		// ���� GUI Style ���϶��ٶȻ������Сֵ
		Vector2 guiParams = GetGUIParamsDefaultValue(guiStyle);

		m_cbInfosDisplay.push_back({ cbElem.name, cbElem.type, cbDataDisplay, guiStyle, guiParams, cbElem.memoryIndex });
	}

	for (auto& texDisplay : m_texInfosDisplay)
	{
		if (texDisplay.pTexture)
			texDisplay.pTexture->RemoveRef();
	}
	m_texInfosDisplay.clear();
	m_texInfosDisplay.reserve(pMaterial->GetTextureCount());
	for (UINT i = 0; i < pMaterial->GetTextureCount(); i++)
	{
		NXTexture* pTex = pMaterial->GetTexture(i);
		pTex->AddRef();
		NXGUITextureType texType = pTex->GetSerializationData().m_textureType == NXTextureType::NormalMap ? NXGUITextureType::Normal : NXGUITextureType::Default;
		m_texInfosDisplay.push_back({ pMaterial->GetTextureName(i), texType, pTex });
	}

	m_nslCodeDisplay = pMaterial->GetNSLCode();

	m_pLastMaterial = pMaterial;
}

NXGUIMaterialShaderEditor* NXGUIMaterial::GetShaderEditor()
{
	return NXGUIMaterialShaderEditor::GetInstance();
}

void NXGUIMaterial::RenderMaterialUI_Custom(NXCustomMaterial* pMaterial)
{
	// TODO: �о� m_pLastMaterial д�����ﲻ̫���ʣ�д������ Update ֮��ĵط������
	// ������������ GUI Update ��ʵ�� ��������ô�����ˡ���
	if (m_pLastMaterial != pMaterial)
		m_bIsDirty = true;

	// ����������ͬ���� GUI������
	if (m_bIsDirty)
	{
		SyncMaterialData(pMaterial);
		m_bIsDirty = false;
	}

	//ImGui::BeginChild("##material_custom", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f));
	{
		// �������ڵ���������
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		RenderMaterialUI_Custom_Parameters(pMaterial);

		//RenderMaterialUI_Custom_Codes(pMaterial); // 2023.6.7 ����Ҫȡ��Codes�ˣ�һֱûɶ�á�����ע���۲�һ��ʱ��
		ImGui::PopStyleVar(); // ImGuiStyleVar_IndentSpacing
		//ImGui::EndChild();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	if (ImGui::TreeNodeEx("Parameters##material_custom_parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("##material_custom_child", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 40.0f));
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

				auto& pTex = texDisplay.pTexture;
				if (!pTex) continue;

				auto onTexChange = [pMaterial, &pTex, this]()
				{
					pMaterial->SetTexture(pTex, m_pFileBrowser->GetSelected());
					RequestSyncMaterialData();
				};

				auto onTexRemove = [pMaterial, &pTex, this]()
				{
					pMaterial->RemoveTexture(pTex);
					RequestSyncMaterialData();
				};

				auto onTexDrop = [pMaterial, &pTex, this](const std::wstring& dragPath)
				{
					pMaterial->SetTexture(pTex, dragPath);
					RequestSyncMaterialData();
				};

				ImGui::PushID(paramCnt);
				RenderSmallTextureIcon(pTex->GetSRV(), m_pFileBrowser, onTexChange, onTexRemove, onTexDrop);
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::Text(texDisplay.name.data());

				paramCnt++;
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

	if (bDraged && cbDisplay.memoryIndex != -1) // �¼ӵ� AddParam �ڵ���밴ť֮ǰ��Ӧ�ô�������
	{
		// �����ｫ GUI �޸Ĺ��Ĳ������ظ����� CBuffer��ʵ���Ӿ��ϵı仯��
		// ʵ����Ҫ�������ֽ����� cbDisplay ��ʼ��ȡ���ֽ����� actualN�������Ǹ��� GUIStyle �Ժ�Ĳ������� N
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
