#include "BaseDefs/DearImGui.h"

#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXScene.h"
#include "NXSubMesh.h"
#include "NXTerrain.h"
#include "NXRenderableObject.h"
#include "NXAllocatorManager.h"
#include "NXGUICommandManager.h"
#include "NXGUITerrainSystem.h"

NXGUIMaterial::NXGUIMaterial(NXScene* pScene, NXGUITerrainSystem* pTerrainSystem) :
	m_pCurrentScene(pScene),
	m_pTerrainSystem(pTerrainSystem),
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

	// ͳ��ѡ�е�����Meshes�����ж��ٲ���
	std::unordered_set<NXMaterial*> pUsedMats;
	for (auto pSubMesh : pPickingSubMeshes)
		pUsedMats.insert(pSubMesh->GetMaterial());

	// ���ѡ�е�����SubMesh��ֻ��һ�����ʣ����˲��ʼ���pUniqueMat
	bool bIsReadOnlyMaterial = pUsedMats.size() != 1;
	NXMaterial* pUniqueMat = bIsReadOnlyMaterial ? nullptr : *pUsedMats.begin();

	bool bIsReadOnlyTransform = pPickingSubMeshes.size() != 1;
	if (bIsReadOnlyTransform) ImGui::BeginDisabled();

	NXRenderableObject* pObject = pPickingSubMeshes[0]->GetRenderableObject();
	NXRenderableObject* pUniqueObj = bIsReadOnlyTransform ? nullptr : pObject;

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

	ImGui::Text("%d Materials, %d Submeshes", pUsedMats.size(), pPickingSubMeshes.size());
	ImGui::Separator();

	if (pUniqueObj)
	{
		RenderGUI_Unique_RenderableObject(pUniqueObj);
	}

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

	if (pUniqueMat)
	{
		ImGui::BeginChild("##material_description", ImVec2(ImGui::GetContentRegionAvail().x, std::max(ImGui::GetContentRegionAvail().y * 0.1f, fBtnSize)));
		std::string strMatName = pUniqueMat->GetName().c_str();
		if (ImGui::InputText("Material", &strMatName))
		{
			pUniqueMat->SetName(strMatName);
		}
		ImGui::EndChild();

		auto pCustomMat = pUniqueMat->IsCustomMat();
		if (pCustomMat)
		{
			RenderMaterialUI_Custom(pCustomMat);

			if (pUniqueMat->IsCustomMat())
			{
				if (ImGui::Button("Edit Shader...##material_custom_editshader"))
				{
					OnBtnEditShaderClicked(pCustomMat);
				}
			}
		}
	}


	// ��Ⱦ Shader Editor GUI
	if (pUniqueMat && pUniqueMat->IsCustomMat())
	{
		if (m_pLastCommonPickMaterial != pUniqueMat)
		{
			NXGUICommand e(NXGUICmd_MSE_SetMaterial, { static_cast<NXCustomMaterial*>(pUniqueMat) });
			NXGUICommandManager::GetInstance()->PushCommand(e);

			m_pLastCommonPickMaterial = pUniqueMat;
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
	// ����clone = ���Ĳ���ȫ��ǳ������
	// �޸Ĵ�m_guiData�ĺ��Ĳ�������ֱ��ӳ�䵽pMaterial��Դ����
	m_guiData.Destroy();
	m_guiData = pMaterial->GetMaterialData().Clone(true);
	m_pLastMaterial = pMaterial;
}

void NXGUIMaterial::RenderGUI_Unique_RenderableObject(NXRenderableObject* pObj)
{
	if (!pObj) return;
	if (auto* pUniqueTerrain = pObj->IsTerrain())
	{
		RenderGUI_Unique_Terrain(pUniqueTerrain);
		return;
	}
}


void NXGUIMaterial::RenderGUI_Unique_Terrain(NXTerrain* pTerrain)
{
	if (!pTerrain) return;

	ImGui::Text("Terrain: %s", pTerrain->GetName().c_str());

	NXTerrainLayer* pTerrainLayer = pTerrain->GetTerrainLayer();
	if (!pTerrainLayer)
	{
		ImGui::Text("[Error] No TerrainLayer pointer.");
		ImGui::Separator();
		return;
	}

	// Layer ·��
	if (!pTerrainLayer->GetPath().empty())
	{
		ImGui::Text("Layer: %s", pTerrainLayer->GetPath().string().c_str());
		RenderGUI_Unique_TerrainLayer(pTerrain, pTerrainLayer);
	}
	else
		ImGui::Text("No valid TerrainLayer.");

	if (ImGui::Button("Switch *.ntl file...##terrainlayer_btn"))
	{
		ImGui::OpenPopup("##terrainlayer_select_popup");
	}

	// Drag-and-Drop�������� .ntl
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
		{
			auto pDropData = static_cast<const NXGUIAssetDragData*>(payload->Data);
			if (pDropData && pDropData->srcPath.has_extension() &&
				NXConvert::IsTerrainLayerExtension(pDropData->srcPath.extension().string()))
			{
				pTerrainLayer->SetPath(pDropData->srcPath);
			}
		}
		ImGui::EndDragDropTarget();
	}

	// ����ѡ�񵯴����ݹ�ö�� D:\\NixAssets ��ȫ�� .ntl
	if (ImGui::BeginPopup("##terrainlayer_select_popup"))
	{
		const std::filesystem::path searchRoot = "D:\\NixAssets";
		for (auto const& entry : std::filesystem::recursive_directory_iterator(searchRoot))
		{
			if (!entry.is_regular_file()) continue;
			if (!NXConvert::IsTerrainLayerExtension(entry.path().extension().string())) continue;

			const std::string fileName = entry.path().filename().string();
			if (ImGui::Selectable(fileName.c_str()))
			{
				pTerrainLayer->SetPath(entry.path());
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}

	if (ImGui::Button("Bake GPU-Driven data##terrainlayer_bake"))
	{
		pTerrainLayer->BakeGPUDrivenData(true);
	}

	if (ImGui::Button("Open Terrain System..."))
	{
		m_pTerrainSystem->Show();
	}

	ImGui::Separator();
}

void NXGUIMaterial::RenderGUI_Unique_TerrainLayer(NXTerrain* pTerrain, NXTerrainLayer* pTerrainLayer)
{
	// Height-Map Сͼ�� + �Ϸ�
	RenderGUI_Unique_TerrainLayer_Connection(pTerrain, pTerrainLayer);

	ImGui::Text("Height Map:");
	ImGui::SameLine();

	auto onHeightMapDrop = [pTerrainLayer](const std::wstring& dragPath) {
			pTerrainLayer->SetHeightMapPath(dragPath);
		};

	std::filesystem::path heightMapPath = pTerrainLayer->GetHeightMapPath();
	if (heightMapPath.empty() || !std::filesystem::exists(heightMapPath))
	{
		heightMapPath = g_defaultTex_white_wstr;
	}

	auto pHeightMapTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("", heightMapPath);
	if (!pHeightMapTex.IsNull())
	{
		NXShVisDescHeap->PushFluid(pHeightMapTex->GetSRV());
		auto& srvHandle = NXShVisDescHeap->Submit();

		using namespace NXGUICommon;
		RenderSmallTextureIcon(srvHandle, nullptr, nullptr, nullptr, onHeightMapDrop, NXGUISmallTexType::Raw);
		ImGui::SameLine();
		ImGui::Text("%s", heightMapPath.filename().string().c_str());
	}
}

void NXGUIMaterial::RenderGUI_Unique_TerrainLayer_Connection(NXTerrain* pTerrain, NXTerrainLayer* pTerrainLayer)
{
	ImGui::PushID("RenderGUI_Unique_TerrainLayer_Connection");

	ImGui::Text("Connection: ");
	ImVec2 regionSize = ImGui::GetContentRegionAvail();
	regionSize.y = std::min(regionSize.y, 120.0f); // �޸�120
	ImGui::BeginChild("TableRegion", regionSize, true); // �����߿�
	int columnCount = 2;
	if (ImGui::BeginTable("RenderGraphTableFlipped", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		// ��ͷ
		ImGui::TableSetupColumn("Resource");
		ImGui::TableSetupColumn("Connection");
		ImGui::TableHeadersRow();

		auto drawRow = [](const char* resourceName, bool state) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(resourceName);
			ImGui::TableSetColumnIndex(1);
			if (state) ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Lost"); // ��ɫ
			else ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected"); // ��ɫ
			};

		// �߶�ͼ
		drawRow("Height Map", pTerrainLayer->GetHeightMapTexture().IsNull());
		// Min/Max Z ͼ
		drawRow("Min/Max Z Map", pTerrainLayer->GetMinMaxZMapTexture().IsNull());
		// Patch Cone ͼ
		drawRow("PatchCone Map", pTerrainLayer->GetPatchConeMapTexture().IsNull());

		ImGui::EndTable();
	}
	ImGui::EndChild();

	ImGui::PopID();
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
