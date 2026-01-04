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
	std::unordered_set<NXMaterial*> pUsedMats;
	for (auto pSubMesh : pPickingSubMeshes)
		pUsedMats.insert(pSubMesh->GetMaterial());

	// 如果选中的所有SubMesh都只有一个材质，将此材质记作pUniqueMat
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


	// 渲染 Shader Editor GUI
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
	// 不调clone = 核心参数全部浅拷贝，
	// 修改此m_guiData的核心参数，会直接映射到pMaterial的源数据
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

	// Layer 路径
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

	// Drag-and-Drop：仅接受 .ntl
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

	// 快速选择弹窗：递归枚举 D:\\NixAssets 下全部 .ntl
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

	ImGui::Separator();
}

void NXGUIMaterial::RenderGUI_Unique_TerrainLayer(NXTerrain* pTerrain, NXTerrainLayer* pTerrainLayer)
{
	// Height-Map 小图标 + 拖放
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
	regionSize.y = std::min(regionSize.y, 120.0f); // 限高120
	ImGui::BeginChild("TableRegion", regionSize, true); // 开启边框
	int columnCount = 2;
	if (ImGui::BeginTable("RenderGraphTableFlipped", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		// 表头
		ImGui::TableSetupColumn("Resource");
		ImGui::TableSetupColumn("Connection");
		ImGui::TableHeadersRow();

		auto drawRow = [](const char* resourceName, bool state) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(resourceName);
			ImGui::TableSetColumnIndex(1);
			if (state) ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Lost"); // 红色
			else ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected"); // 绿色
			};

		// 高度图
		drawRow("Height Map", pTerrainLayer->GetHeightMapTexture().IsNull());

		ImGui::EndTable();
	}
	ImGui::EndChild();

	ImGui::PopID();
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
