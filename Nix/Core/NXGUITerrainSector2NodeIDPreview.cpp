#include "NXGUITerrainSector2NodeIDPreview.h"
#include "Renderer.h"
#include "NXTerrainLODStreamer.h"
#include "NXAllocatorManager.h"
#include "NXTerrainLODStreamConfigs.h"
#include "NXRenderGraph.h"
#include "NXPassMaterialManager.h"
#include "NXPassMaterial.h"

NXGUITerrainSector2NodeIDPreview::NXGUITerrainSector2NodeIDPreview(Renderer* pRenderer) : 
	m_pRenderer(pRenderer)
{
	uint32_t mips = g_terrainStreamConfig.LODSize; // 6
	m_pTexture = NXManager_Tex->CreateTexture2D("TerrainStreaming_Sector2NodeID_Preview", DXGI_FORMAT_R11G11B10_FLOAT, g_terrainStreamConfig.Sector2NodeIDTexSize, g_terrainStreamConfig.Sector2NodeIDTexSize, mips, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pTexture->SetViews(1, 0, 0, mips);
	m_pTexture->SetSRV(0);
	for (int i = 0; i < mips; i++)
	{
		m_pTexture->SetUAV(i, i);
	}

	for (int i = 0; i < mips; i++)
	{
		m_cbMipData[i] = i;
		m_cbMip[i].Set(m_cbMipData[i]);
	}

	m_cbRemapData.remapMin = m_remapMin;
	m_cbRemapData.remapMax = m_remapMax;

	m_pPassMat = new NXComputePassMaterial("TerrainSector2NodePreview", L"Shader\\TerrainSector2NodePreview.fx");
	m_pPassMat->RegisterCBVSpaceNum(1);
	m_pPassMat->RegisterCBVSlotNum(2);  // b0: remap params
	m_pPassMat->RegisterSRVSpaceNum(1);
	m_pPassMat->RegisterSRVSlotNum(6);
	m_pPassMat->RegisterUAVSpaceNum(1);
	m_pPassMat->RegisterUAVSlotNum(6);
	m_pPassMat->FinalizeLayout();
	m_pPassMat->Compile();
}

NXGUITerrainSector2NodeIDPreview::~NXGUITerrainSector2NodeIDPreview()
{
	if (m_pPassMat)
	{
		m_pPassMat->Release();
		delete m_pPassMat;
		m_pPassMat = nullptr;
	}
}

void NXGUITerrainSector2NodeIDPreview::Update()
{
	if (!m_bVisible)
		return;

	auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
	if (!pStreamer) return;

	auto& pStreamData = pStreamer->GetStreamingData();
	auto pSrcTexture = pStreamData.GetSector2NodeIDTexture();
	if (!pSrcTexture.IsValid()) return;

	m_cbRemapData.remapMin = m_remapMin;
	m_cbRemapData.remapMax = m_remapMax;
	m_cbRemap.Update(m_cbRemapData);

	struct PassData
	{
		NXRGHandle pIn;
		NXRGHandle pOut;
	};

	auto rg = m_pRenderer->GetRenderGraph();
	if (rg && m_pPassMat)
	{
		// 为当前选中的 mip 等级执行 Compute Shader
		rg->AddPass<PassData>("TerrainSector2NodeID Preview Pass",
			[&](NXRGBuilder& builder, PassData& data) 
			{
				data.pIn = builder.Read(rg->Import(pSrcTexture));
				data.pOut = builder.Write(rg->Import(m_pTexture));
			},
			[this](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PassData& data)
			{
				m_pPassMat->SetConstantBuffer(0, 0, &m_cbRemap);

				m_pPassMat->SetInput(0, 0, resMap.GetRes(data.pIn));
				//for (int mip = 0; mip < g_terrainStreamConfig.LODSize; mip++)
				int mip = m_currentMipLevel;
				{
					m_pPassMat->SetOutput(0, mip, resMap.GetRes(data.pOut), mip);

					m_pPassMat->SetConstantBuffer(0, 1, &m_cbMip[mip]);

					m_pPassMat->RenderSetTargetAndState(pCmdList);
					m_pPassMat->RenderBefore(pCmdList);

					uint32_t mipSize = g_terrainStreamConfig.Sector2NodeIDTexSize >> mip;
					uint32_t threadGroups = (mipSize + 7) / 8;
					pCmdList->Dispatch(threadGroups, threadGroups, 1);
				}
			});
	}
}

void NXGUITerrainSector2NodeIDPreview::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(600.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("地形 Sector2NodeID 预览"), &m_bVisible))
	{
		if (m_pRenderer)
		{
			auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
			if (pStreamer)
			{
				auto& streamData = pStreamer->GetStreamingData();
				auto pSrcTexture = streamData.GetSector2NodeIDTexture();

				if (pSrcTexture.IsValid() && m_pTexture.IsValid())
				{
					// 使用两列布局
					ImGui::Columns(2, "PreviewColumns", true);

					// ========== 左列：纹理预览 ==========
					{
						ImGui::Text(ImUtf8("预览 (Mip %d)"), m_currentMipLevel);
						ImGui::Separator();

						// 计算当前 mip 等级的纹理尺寸（源纹理的 mip 尺寸）
						uint32_t mipSize = g_terrainStreamConfig.Sector2NodeIDTexSize >> m_currentMipLevel;
						float texWidth = (float)mipSize;
						float texHeight = (float)mipSize;

						// 获取可用区域
						float availWidth = ImGui::GetContentRegionAvail().x - 10.0f;
						float availHeight = ImGui::GetContentRegionAvail().y - 10.0f;

						// 计算显示尺寸（保持纵横比）
						float aspect = texWidth / texHeight;
						float displayWidth = availWidth;
						float displayHeight = displayWidth / aspect;

						// 如果高度超出可用区域，则按高度缩放
						if (displayHeight > availHeight)
						{
							displayHeight = availHeight;
							displayWidth = displayHeight * aspect;
						}

						// 滚动区域以支持缩放后的查看
						ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

						// 获取 SRV 并渲染
						// 计算 UV 范围：源 mip 的内容写入到 mip0 的左上角
						// UV 范围 = mipSize / fullSize
						float uvMax = (float)mipSize / (float)g_terrainStreamConfig.Sector2NodeIDTexSize;
						NXShVisDescHeap->PushFluid(m_pTexture->GetSRV());
						D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = NXShVisDescHeap->Submit();
						ImGui::Image((ImTextureID)srvHandle.ptr, ImVec2(displayWidth, displayHeight), 
							ImVec2(0.0f, 0.0f), ImVec2(uvMax, uvMax));

						ImGui::EndChild();
					}

					// ========== 右列：参数控制 ==========
					ImGui::NextColumn();
					{
						ImGui::Text(ImUtf8("参数设置"));
						ImGui::Separator();

						// 显示源纹理信息
						ImGui::Text(ImUtf8("源纹理信息:"));
						ImGui::Text(ImUtf8("  尺寸: %d x %d"), pSrcTexture->GetWidth(), pSrcTexture->GetHeight());
						ImGui::Text(ImUtf8("  格式: R16_UINT"));
						ImGui::Text(ImUtf8("  Mip 层级数: %d"), g_terrainStreamConfig.LODSize);

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// 1. Mip 等级选择
						ImGui::Text(ImUtf8("Mip 层级:"));
						ImGui::SliderInt("##MipLevel", &m_currentMipLevel, 0, g_terrainStreamConfig.LODSize - 1, "Mip %d");
						uint32_t currentMipSize = g_terrainStreamConfig.Sector2NodeIDTexSize >> m_currentMipLevel;
						ImGui::Text(ImUtf8("  当前 Mip 尺寸: %d x %d"), currentMipSize, currentMipSize);

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// 2. Remap 参数控制
						ImGui::Text(ImUtf8("重映射范围 (NodeID -> [0,1]):"));
						ImGui::DragFloatRange2("##RemapRange", &m_remapMin, &m_remapMax, 1.0f, 0.0f, 1024.0f, ImUtf8("最小: %.0f"), ImUtf8("最大: %.0f"));
						
						ImGui::Spacing();
						if (ImGui::Button(ImUtf8("重置")))
						{
							m_remapMin = 0.0f;
							m_remapMax = 1024.0f;
						}
						ImGui::SameLine();
						if (ImGui::Button("0-256"))
						{
							m_remapMin = 0.0f;
							m_remapMax = 256.0f;
						}
						ImGui::SameLine();
						if (ImGui::Button("0-512"))
						{
							m_remapMin = 0.0f;
							m_remapMax = 512.0f;
						}

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// 显示当前 remap 说明
						ImGui::TextWrapped(ImUtf8("NodeID 值在 [%.0f, %.0f] 范围内将被映射到灰度 [0, 1]。"), m_remapMin, m_remapMax);
						ImGui::TextWrapped(ImUtf8("超出此范围的值将被截断。"));
						ImGui::TextWrapped(ImUtf8("无效像素 (0xFFFF) 显示为黑色。"));
					}

					ImGui::Columns(1);
				}
				else
				{
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("纹理不可用"));
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("TerrainLODStreamer 未初始化"));
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImUtf8("渲染器未设置"));
		}
	}
	ImGui::End();
}
