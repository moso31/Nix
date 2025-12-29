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
	// 预览纹理不需要mip
	m_pTexture = NXManager_Tex->CreateTexture2D("TerrainStreaming_Sector2NodeID", DXGI_FORMAT_R16_UINT, g_terrainStreamConfig.AtlasHeightMapSize, g_terrainStreamConfig.AtlasHeightMapSize, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	std::string matName = "GUI Terrain Sector2NodeID Texture Preview";
	NXPassMaterial* pMat = new NXComputePassMaterial(matName);
	NXPassMng->AddMaterial("GUI Terrain Sector2NodeID Texture Preview", );
}

NXGUITerrainSector2NodeIDPreview::~NXGUITerrainSector2NodeIDPreview()
{
}

void NXGUITerrainSector2NodeIDPreview::Update()
{
	if (!m_bVisible)
		return;

	auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
	if (!pStreamer) return;

	auto& pStreamData = pStreamer->GetStreamingData();

	struct PassData
	{
		NXRGHandle pIn;
		NXRGHandle pOut;
	};

	auto rg = m_pRenderer->GetRenderGraph();
	if (rg)
	{
		rg->AddPass<PassData>("TerrainSector2NodeID preview pass",
			[&](NXRGBuilder& builder, PassData& data) 
			{
				data.pIn = builder.Read(rg->Import(pStreamData.GetSector2NodeIDTexture()));
				data.pOut = builder.Write(rg->Import(m_pTexture));
			},
			[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PassData& data)
			{
			});
	}
}

void NXGUITerrainSector2NodeIDPreview::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(400.0f, 450.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("Terrain Sector2NodeID Preview"), &m_bVisible))
	{
		if (m_pRenderer)
		{
			auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
			if (pStreamer)
			{
				auto& streamData = pStreamer->GetStreamingData();
				auto pTexture = streamData.GetSector2NodeIDTexture();

				if (pTexture.IsValid())
				{
					// 缩放控制
					ImGui::SliderFloat("Zoom", &m_zoomScale, 0.25f, 4.0f, "%.2fx");
					ImGui::SameLine();
					if (ImGui::Button("Reset")) m_zoomScale = 1.0f;

					ImGui::Separator();

					// 显示纹理信息
					ImGui::Text("Size: %d x %d", pTexture->GetWidth(), pTexture->GetHeight());
					ImGui::Text("Format: R16_UINT (NodeID)");

					ImGui::Separator();

					// 渲染纹理预览
					float texWidth = (float)pTexture->GetWidth();
					float texHeight = (float)pTexture->GetHeight();
					float availWidth = ImGui::GetContentRegionAvail().x;
					float availHeight = ImGui::GetContentRegionAvail().y;

					// 计算显示尺寸（保持纵横比）
					float aspect = texWidth / texHeight;
					float displayWidth = availWidth * m_zoomScale;
					float displayHeight = displayWidth / aspect;

					// 如果高度超出可用区域，则按高度缩放
					if (displayHeight > availHeight * m_zoomScale)
					{
						displayHeight = availHeight * m_zoomScale;
						displayWidth = displayHeight * aspect;
					}

					// 滚动区域以支持缩放后的查看
					ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

					// 获取SRV并渲染
					NXShVisDescHeap->PushFluid(pTexture->GetSRV());
					D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = NXShVisDescHeap->Submit();
					ImGui::Image((ImTextureID)srvHandle.ptr, ImVec2(displayWidth, displayHeight));

					ImGui::EndChild();
				}
				else
				{
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Texture not available");
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "TerrainLODStreamer not initialized");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Renderer not set");
		}
	}
	ImGui::End();
}
