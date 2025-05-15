#include "BaseDefs/DearImGui.h"

#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXScene.h"
#include "NXSubMesh.h"
#include "NXRenderableObject.h"
#include "NXAllocatorManager.h"
#include "NXGUICommandManager.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene) :
	m_pCurrentScene(pScene),
	m_bIsDirty(false)
{
}

void NXGUIMaterial::Render()
{
	ImGui::Text("Material");

	std::vector<NXSubMeshBase*> pPickingSubMeshes = m_pCurrentScene->GetPickingSubMeshes();
	if (pPickingSubMeshes.empty())
	{
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

	NXRenderableObject* pObject = pPickingSubMeshes[0]->GetRenderableObject();
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
	ImGui::BeginChild("##material_iconbtn", ImVec2(fBtnSize, std::max(ImGui::GetContentRegionAvail().y * 0.1f, fBtnSize)));
	ImGui::Button(".nsl##iconbtn", ImVec2(fBtnSize, fBtnSize));

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
		ImGui::BeginChild("##material_description", ImVec2(ImGui::GetContentRegionAvail().x, std::max(ImGui::GetContentRegionAvail().y * 0.1f, fBtnSize)));
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

			if (pCommonMaterial->IsCustomMat())
			{
				if (ImGui::Button("Edit Shader...##material_custom_editshader"))
				{
					OnBtnEditShaderClicked(pCustomMat);
				}
			}
		}
	}


	// 渲染 Shader Editor GUI
	if (pCommonMaterial && pCommonMaterial->IsCustomMat())
	{
		if (m_pLastCommonPickMaterial != pCommonMaterial)
		{
			NXGUICommand e(NXGUICmd_MSE_SetMaterial, { static_cast<NXCustomMaterial*>(pCommonMaterial) });
			NXGUICommandManager::GetInstance()->PushCommand(e);

			m_pLastCommonPickMaterial = pCommonMaterial;
		}
	}
}

void NXGUIMaterial::Release()
{
	m_guiData.Destroy();
}

void NXGUIMaterial::OnBtnEditShaderClicked(NXCustomMaterial* pMaterial)
{
	NXGUICommand e(NXGUICmd_Inspector_OpenShaderEditor);
	NXGUICommandManager::GetInstance()->PushCommand(e);
}

void NXGUIMaterial::SyncMaterialData(NXCustomMaterial* pMaterial)
{
	// 不调clone = 核心参数全部浅拷贝，
	// 修改此m_guiData的核心参数，会直接映射到pMaterial的源数据
	m_guiData.Destroy();
	m_guiData = pMaterial->GetMaterialData().Clone(true);
	m_pLastMaterial = pMaterial;
}

void NXGUIMaterial::RenderMaterialUI_Custom(NXCustomMaterial* pMaterial)
{
	// TODO: 感觉 m_pLastMaterial 写在这里不太合适，写在类似 Update 之类的地方会更好
	// 但现在懒得做 GUI Update 的实现 所以先这么放着了……
	if (m_pLastMaterial != pMaterial)
		m_bIsDirty = true;

	// 将材质数据同步到 GUI材质类
	if (m_bIsDirty)
	{
		SyncMaterialData(pMaterial);
		m_bIsDirty = false;
	}

	//ImGui::BeginChild("##material_custom", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f));
	{
		// 禁用树节点首行缩进
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		RenderMaterialUI_Custom_Parameters(pMaterial);

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

			for (auto* guiData : m_guiData.GetAll())
			{
				if (auto* cbData = guiData->IsCBuffer())
				{
					std::string strId = "##material_custom_child_cbuffer_" + std::to_string(paramCnt++);
					RenderMaterialUI_Custom_Parameters_CBufferItem(strId, pMaterial, cbData);
				}
				else if (auto* txData = guiData->IsTexture())
				{
					std::string strId = "##material_custom_child_texture_" + std::to_string(paramCnt);

					auto& pTex = txData->pTexture;
					if (pTex.IsNull()) continue;

					auto onTexChange = [pMaterial, &pTex, this]()
						{
							//pMaterial->SetTexture(pTex, m_pFileBrowser->GetSelected());
							RequestSyncMaterialData();
						};

					auto onTexRemove = [pMaterial, &pTex, this]()
						{
							pMaterial->RemoveTexture(pTex);
							RequestSyncMaterialData();
						};

					auto onTexDrop = [txData, &pTex, this](const std::wstring& dragPath)
						{
							txData->pTexture = pTex;
							txData->SyncLink();
							RequestSyncMaterialData();
						};

					ImGui::PushID(paramCnt);
					NXShVisDescHeap->PushFluid(pTex->GetSRV());
					auto& srvHandle = NXShVisDescHeap->Submit();
					RenderSmallTextureIcon(srvHandle, nullptr, onTexChange, onTexRemove, onTexDrop);
					ImGui::PopID();

					ImGui::SameLine();
					ImGui::Text(txData->name.c_str());

					paramCnt++;
				}
			}

			ImGui::EndChild();
		}
		ImGui::TreePop();
	}
}

void NXGUIMaterial::RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMatDataCBuffer* cbData)
{
	using namespace NXGUICommon;

	bool bDraged = false;
	std::string strName = cbData->name + strId + "_cb";

	switch (cbData->gui.style)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		bDraged |= ImGui::DragScalarN(strName.data(), ImGuiDataType_Float, cbData->data, cbData->size, cbData->gui.params[0]);
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		bDraged |= ImGui::SliderScalarN(strName.data(), ImGuiDataType_Float, cbData->data, cbData->size, &cbData->gui.params[0], &cbData->gui.params[1]);
		break;
	case NXGUICBufferStyle::Color3:
		bDraged |= ImGui::ColorEdit3(strName.data(), cbData->data);
		break;
	case NXGUICBufferStyle::Color4:
		bDraged |= ImGui::ColorEdit4(strName.data(), cbData->data);
		break;
	}

	if (bDraged)
	{
		cbData->SyncLink();
		pMaterial->SetCBInfoMemoryData();
	}
}
