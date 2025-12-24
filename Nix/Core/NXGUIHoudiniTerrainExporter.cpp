#include "NXGUIHoudiniTerrainExporter.h"
#include <DirectXTex.h>
#include <algorithm>
#include <regex>
#include <fstream>
#include <cmath>
#include "tinyexr/tinyexr.h"
#include "NXTextureMaker.h"

using namespace DirectX;

NXGUIHoudiniTerrainExporter::NXGUIHoudiniTerrainExporter()
{
}

NXGUIHoudiniTerrainExporter::~NXGUIHoudiniTerrainExporter()
{
}

void NXGUIHoudiniTerrainExporter::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(1200.0f, 600.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("Houdini 地形导出器"), &m_bVisible))
	{
		// 获取可用宽度，四列平均分配
		float availWidth = ImGui::GetContentRegionAvail().x;
		float columnWidth = (availWidth - 30.0f) / 4.0f; // 减去间隔

		// 第一列
		ImGui::BeginChild("Column1", ImVec2(columnWidth, 0), true);
		RenderColumn1_HoudiniFiles();
		ImGui::EndChild();

		ImGui::SameLine();

		// 第二列
		ImGui::BeginChild("Column2", ImVec2(columnWidth, 0), true);
		RenderColumn2_ConvertToDDS();
		ImGui::EndChild();

		ImGui::SameLine();

		// 第三列
		ImGui::BeginChild("Column3", ImVec2(columnWidth, 0), true);
		RenderColumn3_BakeSubTiles();
		ImGui::EndChild();

		ImGui::SameLine();

		// 第四列
		ImGui::BeginChild("Column4", ImVec2(columnWidth, 0), true);
		RenderColumn4_Compose2DArray();
		ImGui::EndChild();
	}
	ImGui::End();
}

void NXGUIHoudiniTerrainExporter::RenderColumn1_HoudiniFiles()
{
	ImGui::Text(ImUtf8("=== Houdini EXR 文件 ==="));
	ImGui::Separator();

	// 路径输入
	ImGui::Text(ImUtf8("Houdini纹理路径:"));
	float inputWidth = ImGui::GetContentRegionAvail().x - 80.0f;
	ImGui::SetNextItemWidth(inputWidth);
	ImGui::InputText("##HoudiniPath", m_houdiniBasePath, sizeof(m_houdiniBasePath));
	ImGui::SameLine();
	if (ImGui::Button(ImUtf8("获取纹理")))
	{
		ScanHoudiniExrFiles();
	}

	ImGui::Spacing();

	// 显示纹理数量
	ImGui::Text(ImUtf8("Height纹理: %d 个"), (int)m_heightExrFiles.size());
	ImGui::Text(ImUtf8("Splat纹理: %d 个"), (int)m_splatExrFiles.size());

	ImGui::Spacing();
	ImGui::Separator();

	// Height文件下拉菜单
	ImGui::Text(ImUtf8("Height文件列表:"));
	if (!m_heightExrFiles.empty())
	{
		ImGui::SetNextItemWidth(-1);
		if (ImGui::BeginCombo("##HeightFiles", m_heightExrFiles[m_selectedHeightExrIndex].fileName.c_str()))
		{
			for (int i = 0; i < (int)m_heightExrFiles.size(); i++)
			{
				bool isSelected = (m_selectedHeightExrIndex == i);
				if (ImGui::Selectable(m_heightExrFiles[i].fileName.c_str(), isSelected))
				{
					m_selectedHeightExrIndex = i;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}
	else
	{
		ImGui::TextDisabled(ImUtf8("(无文件)"));
	}

	ImGui::Spacing();

	// Splat文件下拉菜单
	ImGui::Text(ImUtf8("Splat文件列表:"));
	if (!m_splatExrFiles.empty())
	{
		ImGui::SetNextItemWidth(-1);
		if (ImGui::BeginCombo("##SplatFiles", m_splatExrFiles[m_selectedSplatExrIndex].fileName.c_str()))
		{
			for (int i = 0; i < (int)m_splatExrFiles.size(); i++)
			{
				bool isSelected = (m_selectedSplatExrIndex == i);
				if (ImGui::Selectable(m_splatExrFiles[i].fileName.c_str(), isSelected))
				{
					m_selectedSplatExrIndex = i;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}
	else
	{
		ImGui::TextDisabled(ImUtf8("(无文件)"));
	}
}

void NXGUIHoudiniTerrainExporter::RenderColumn2_ConvertToDDS()
{
	ImGui::Text(ImUtf8("=== 转录为 DDS ==="));
	ImGui::Separator();

	// 输出路径
	ImGui::Text(ImUtf8("Nix输出路径:"));
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##NixOutputPath", m_nixOutputPath, sizeof(m_nixOutputPath));

	ImGui::Spacing();

	// 转换选项
	ImGui::Checkbox(ImUtf8("转换 HeightMap"), &m_bConvertHeightMap);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::InputInt2(ImUtf8("高度范围"), m_heightMapRange);
	ImGui::Checkbox(ImUtf8("转换 SplatMap"), &m_bConvertSplatMap);

	ImGui::Spacing();

	// 转换按钮
	bool canConvert = !m_heightExrFiles.empty() || !m_splatExrFiles.empty();
	if (!canConvert)
		ImGui::BeginDisabled();
	
	if (ImGui::Button(ImUtf8("开始转换"), ImVec2(-1, 30)))
	{
		ExecuteConvertAll();
	}
	
	if (!canConvert)
		ImGui::EndDisabled();

	ImGui::Spacing();
	ImGui::Separator();

	// 刷新已转换文件列表
	if (ImGui::Button(ImUtf8("刷新DDS列表")))
	{
		ScanNixDdsFiles();
	}

	ImGui::Spacing();

	// 显示已转换的DDS文件
	ImGui::Text(ImUtf8("已转换的DDS: %d 组"), (int)m_nixDdsFiles.size());
	
	if (!m_nixDdsFiles.empty())
	{
		ImGui::SetNextItemWidth(-1);
		if (ImGui::BeginCombo("##DdsFiles", m_nixDdsFiles[m_selectedDdsIndex].terrainId.c_str()))
		{
			for (int i = 0; i < (int)m_nixDdsFiles.size(); i++)
			{
				bool isSelected = (m_selectedDdsIndex == i);
				if (ImGui::Selectable(m_nixDdsFiles[i].terrainId.c_str(), isSelected))
				{
					m_selectedDdsIndex = i;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// 显示选中文件的详细信息
		if (m_selectedDdsIndex < (int)m_nixDdsFiles.size())
		{
			const auto& dds = m_nixDdsFiles[m_selectedDdsIndex];
			ImGui::TextWrapped(ImUtf8("HeightMap: %s"), dds.heightMapPath.string().c_str());
			ImGui::TextWrapped(ImUtf8("SplatMap: %s"), dds.splatMapPath.string().c_str());
		}
	}
	else
	{
		ImGui::TextDisabled(ImUtf8("(无文件)"));
	}
}

void NXGUIHoudiniTerrainExporter::RenderColumn3_BakeSubTiles()
{
	ImGui::Text(ImUtf8("=== 烘焙子区域纹理 ==="));
	ImGui::Separator();

	// 输出路径
	ImGui::Text(ImUtf8("子区域纹理输出根路径:"));
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##SubTileOutputPath", m_subTileOutputPath, sizeof(m_subTileOutputPath));

	ImGui::Spacing();
	ImGui::TextDisabled(ImUtf8("输出目录结构: <根路径>\\X_Y\\sub\\hmap\\ 或 splat\\"));

	ImGui::Spacing();
	ImGui::Separator();

	// 烘焙选项
	ImGui::Text(ImUtf8("烘焙选项:"));
	ImGui::Checkbox(ImUtf8("烘焙 HeightMap 子区域"), &m_bBakeHeightMapSubTiles);
	ImGui::Checkbox(ImUtf8("烘焙 SplatMap 子区域"), &m_bBakeSplatMapSubTiles);

	ImGui::Spacing();

	// 强制生成选项
	ImGui::Checkbox(ImUtf8("强制生成（忽略已存在的文件）"), &m_bForceGenerateSubTiles);

	if (m_bForceGenerateSubTiles)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ImUtf8("将重新生成所有文件！"));
	}
	else
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImUtf8("将跳过已存在的文件"));
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 检查是否有DDS文件可供烘焙
	bool canBake = !m_nixDdsFiles.empty() && (m_bBakeHeightMapSubTiles || m_bBakeSplatMapSubTiles);
	if (!canBake)
		ImGui::BeginDisabled();

	if (ImGui::Button(ImUtf8("开始烘焙子区域纹理"), ImVec2(-1, 30)))
	{
		TerrainTexLODBakeConfig bakeConfig;
		bakeConfig.bForceGenerate = m_bForceGenerateSubTiles;
		bakeConfig.bGenerateHeightMap = m_bBakeHeightMapSubTiles;
		bakeConfig.bGenerateSplatMap = m_bBakeSplatMapSubTiles;

		// 遍历所有DDS文件，构建烘焙数据
		for (const auto& ddsInfo : m_nixDdsFiles)
		{
			PerTerrainBakeData perTerrainBakeData;
			perTerrainBakeData.nodeId = { (short)ddsInfo.tileX, (short)ddsInfo.tileY };

			// 构建路径：<根路径>\X_Y\hmap.dds 和 <根路径>\X_Y\splatmap.dds
			std::filesystem::path basePath(m_subTileOutputPath);
			std::string terrainId = std::to_string(ddsInfo.tileX) + "_" + std::to_string(ddsInfo.tileY);
			std::filesystem::path terrainDir = basePath / terrainId;

			perTerrainBakeData.pathHeightMap = terrainDir / "hmap.dds";
			perTerrainBakeData.pathSplatMap = terrainDir / "splatmap.dds";

			bakeConfig.bakeTerrains.push_back(perTerrainBakeData);
		}

		// 调用烘焙函数
		NXTextureMaker::GenerateTerrainStreamingLODMaps(bakeConfig);
	}

	if (!canBake)
		ImGui::EndDisabled();

	ImGui::Spacing();

	// 显示帮助信息
	ImGui::TextWrapped(ImUtf8("说明：此功能将DDS纹理切分成若干子区域(subtile)，用于支持地形流式加载。每个地形将生成多级LOD的子纹理块。"));
}

void NXGUIHoudiniTerrainExporter::RenderColumn4_Compose2DArray()
{
	ImGui::Text(ImUtf8("=== 合成 2D Array ==="));
	ImGui::Separator();

	// HeightArray路径
	ImGui::Checkbox(ImUtf8("合成 HeightMap 2DArray"), &m_bComposeHeightArray);
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##HeightArrayPath", m_heightArrayPath, sizeof(m_heightArrayPath));

	ImGui::Spacing();

	// MinMaxZ路径
	ImGui::Checkbox(ImUtf8("合成 MinMaxZ"), &m_bComposeMinMaxZ);
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##MinMaxZPath", m_minMaxZPath, sizeof(m_minMaxZPath));

	ImGui::Spacing();

	// SplatMap 2DArray路径
	ImGui::Checkbox(ImUtf8("合成 SplatMap 2DArray"), &m_bComposeSplatArray);
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##SplatArrayPath", m_splatArrayPath, sizeof(m_splatArrayPath));

	ImGui::Spacing();
	ImGui::Separator();

	// 排序设置
	ImGui::Text(ImUtf8("排序设置:"));
	ImGui::Checkbox(ImUtf8("行正序 (Y+)"), &m_bRowAscending);
	ImGui::Checkbox(ImUtf8("列正序 (X+)"), &m_bColAscending);
	ImGui::Checkbox(ImUtf8("先遍历列（不勾选则行优先）"), &m_bColumnFirst);

	ImGui::Spacing();

	// 地形范围信息(自动计算)
	ImGui::Text(ImUtf8("地形范围: %d x %d"), m_terrainCountX, m_terrainCountY);

	ImGui::Spacing();
	ImGui::Separator();

	// 开始合成按钮
	bool canCompose = !m_nixDdsFiles.empty();
	if (!canCompose)
		ImGui::BeginDisabled();

	if (ImGui::Button(ImUtf8("开始合成"), ImVec2(-1, 30)))
	{
		if (m_bComposeHeightArray)
			ComposeHeightMap2DArray();
		if (m_bComposeMinMaxZ)
			ComposeMinMaxZ2DArray();
		if (m_bComposeSplatArray)
			ComposeSplatMap2DArray();
	}

	if (!canCompose)
		ImGui::EndDisabled();

	ImGui::Spacing();
	ImGui::Separator();

	// 预览列表
	ImGui::Text(ImUtf8("Slice索引预览:"));
	
	if (!m_nixDdsFiles.empty())
	{
		auto sortedIndices = GetSortedSliceIndices();
		
		ImGui::BeginChild("SlicePreview", ImVec2(-1, 150), true);
		for (int i = 0; i < (int)sortedIndices.size(); i++)
		{
			int idx = sortedIndices[i];
			if (idx >= 0 && idx < (int)m_nixDdsFiles.size())
			{
				ImGui::Text("Slice[%d]: %s", i, m_nixDdsFiles[idx].terrainId.c_str());
			}
		}
		ImGui::EndChild();
	}
	else
	{
		ImGui::TextDisabled(ImUtf8("(请先刷新DDS列表)"));
	}
}

void NXGUIHoudiniTerrainExporter::ScanHoudiniExrFiles()
{
	m_heightExrFiles.clear();
	m_splatExrFiles.clear();
	m_selectedHeightExrIndex = 0;
	m_selectedSplatExrIndex = 0;

	std::filesystem::path basePath(m_houdiniBasePath);

	// 扫描height文件夹
	std::filesystem::path heightDir = basePath / "height";
	if (std::filesystem::exists(heightDir) && std::filesystem::is_directory(heightDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(heightDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".exr")
			{
				HoudiniExrFileInfo info;
				info.fullPath = entry.path();
				info.fileName = entry.path().filename().string();
				ParseTileCoord(info.fileName, info.tileX, info.tileY);
				m_heightExrFiles.push_back(info);
			}
		}
	}

	// 扫描splat文件夹
	std::filesystem::path splatDir = basePath / "splat";
	if (std::filesystem::exists(splatDir) && std::filesystem::is_directory(splatDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(splatDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".exr")
			{
				HoudiniExrFileInfo info;
				info.fullPath = entry.path();
				info.fileName = entry.path().filename().string();
				ParseTileCoord(info.fileName, info.tileX, info.tileY);
				m_splatExrFiles.push_back(info);
			}
		}
	}

	//// 按坐标排序
	//auto sortByCoord = [](const HoudiniExrFileInfo& a, const HoudiniExrFileInfo& b) {
	//	if (a.tileY != b.tileY) return a.tileY < b.tileY;
	//	return a.tileX < b.tileX;
	//};
	//std::sort(m_heightExrFiles.begin(), m_heightExrFiles.end(), sortByCoord);
	//std::sort(m_splatExrFiles.begin(), m_splatExrFiles.end(), sortByCoord);
}

void NXGUIHoudiniTerrainExporter::ScanNixDdsFiles()
{
	m_nixDdsFiles.clear();
	m_selectedDdsIndex = 0;
	m_terrainCountX = 0;
	m_terrainCountY = 0;

	std::filesystem::path basePath(m_nixOutputPath);
	if (!std::filesystem::exists(basePath))
		return;

	int minX = INT_MAX, maxX = INT_MIN;
	int minY = INT_MAX, maxY = INT_MIN;

	for (const auto& entry : std::filesystem::directory_iterator(basePath))
	{
		if (!entry.is_directory())
			continue;

		std::string dirName = entry.path().filename().string();
		int x, y;
		if (!ParseTileCoord(dirName + ".exr", x, y)) // 复用解析逻辑，加个假后缀
			continue;

		NixTerrainDdsInfo info;
		info.terrainId = dirName;
		info.tileX = x;
		info.tileY = y;
		info.heightMapPath = entry.path() / "hmap.dds";
		info.splatMapPath = entry.path() / "splatmap.dds";

		// 检查文件是否存在
		if (std::filesystem::exists(info.heightMapPath) || std::filesystem::exists(info.splatMapPath))
		{
			m_nixDdsFiles.push_back(info);

			minX = std::min(minX, x);
			maxX = std::max(maxX, x);
			minY = std::min(minY, y);
			maxY = std::max(maxY, y);
		}
	}

	// 按坐标排序
	std::sort(m_nixDdsFiles.begin(), m_nixDdsFiles.end(), [](const NixTerrainDdsInfo& a, const NixTerrainDdsInfo& b) {
		if (a.tileY != b.tileY) return a.tileY < b.tileY;
		return a.tileX < b.tileX;
	});

	// 计算地形范围
	if (!m_nixDdsFiles.empty())
	{
		m_terrainCountX = maxX - minX + 1;
		m_terrainCountY = maxY - minY + 1;
	}
}

void NXGUIHoudiniTerrainExporter::ConvertExrToHeightMapDDS(const HoudiniExrFileInfo& exrInfo, const std::filesystem::path& outPath)
{
	// 使用tinyexr读取EXR文件 (32位浮点RGBA)
	float* exrData = nullptr;
	int width, height;
	const char* err = nullptr;
	
	int ret = LoadEXR(&exrData, &width, &height, exrInfo.fullPath.string().c_str(), &err);
	if (ret != TINYEXR_SUCCESS) 
	{
		if (err) 
		{
			printf("EXR加载失败: %s\n", err);
			FreeEXRErrorMessage(err);
		}
		return;
	}

	// 创建R16_UNORM纹理
	ScratchImage img;
	HRESULT hr = img.Initialize2D(DXGI_FORMAT_R16_UNORM, width, height, 1, 1);
	if (FAILED(hr))
	{
		printf("ConvertExrToHeightMapDDS: Initialize2D 失败\n");
		free(exrData);
		return;
	}
	
	const Image* dst = img.GetImage(0, 0, 0);
	
	// EXR是RGBA格式，stride=4
	// 输入值已经是0..1区间，直接映射到R16_UNORM
	for (int y = 0; y < height; ++y) 
	{
		uint16_t* dstRow = reinterpret_cast<uint16_t*>(dst->pixels + y * dst->rowPitch);
		for (int x = 0; x < width; ++x) 
		{
			int srcIdx = (y * width + x) * 4; // RGBA
			float heightVal = exrData[srcIdx]; // R通道是高度（已经是0..1）
			
			// 输入已经是0..1，直接clamp并转换为R16_UNORM
			float normalized = std::clamp(heightVal, 0.0f, 1.0f);
			dstRow[x] = static_cast<uint16_t>(normalized * 65535.0f);
		}
	}
	
	free(exrData);
	
	// 确保目录存在
	std::filesystem::create_directories(outPath.parent_path());
	
	// 保存DDS
	hr = SaveToDDSFile(img.GetImages(), img.GetImageCount(), img.GetMetadata(), 
				  DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ConvertExrToHeightMapDDS: 保存DDS失败 %s\n", outPath.string().c_str());
	}
	else
	{
		printf("ConvertExrToHeightMapDDS: %s -> %s 成功\n", 
			   exrInfo.fullPath.string().c_str(), outPath.string().c_str());
	}
}

void NXGUIHoudiniTerrainExporter::ConvertExrToSplatMapDDS(const HoudiniExrFileInfo& exrInfo, const std::filesystem::path& outPath)
{
	// 使用tinyexr读取EXR文件 (32位浮点RGBA)
	float* exrData = nullptr;
	int width, height;
	const char* err = nullptr;
	
	int ret = LoadEXR(&exrData, &width, &height, exrInfo.fullPath.string().c_str(), &err);
	if (ret != TINYEXR_SUCCESS) 
	{
		if (err) 
		{
			printf("EXR加载失败: %s\n", err);
			FreeEXRErrorMessage(err);
		}
		return;
	}

	// 创建R8_UNORM纹理
	ScratchImage img;
	HRESULT hr = img.Initialize2D(DXGI_FORMAT_R8_UNORM, width, height, 1, 1);
	if (FAILED(hr))
	{
		printf("ConvertExrToSplatMapDDS: Initialize2D 失败\n");
		free(exrData);
		return;
	}
	
	const Image* dst = img.GetImage(0, 0, 0);
	
	// EXR是RGBA格式，stride=4
	// 输入值已经是0..1区间，直接映射到R8_UNORM
	for (int y = 0; y < height; ++y) 
	{
		uint8_t* dstRow = dst->pixels + y * dst->rowPitch;
		for (int x = 0; x < width; ++x) 
		{
			int srcIdx = (y * width + x) * 4; // RGBA
			float idVal = exrData[srcIdx]; // R通道是材质ID（已经是0..1）
			
			// 输入已经是0..1，直接clamp并转换为R8_UNORM
			float normalized = std::clamp(idVal, 0.0f, 1.0f);
			dstRow[x] = static_cast<uint8_t>(normalized * 255.0f);
		}
	}
	
	free(exrData);
	
	// 确保目录存在
	std::filesystem::create_directories(outPath.parent_path());
	
	// 保存DDS
	hr = SaveToDDSFile(img.GetImages(), img.GetImageCount(), img.GetMetadata(), 
				  DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ConvertExrToSplatMapDDS: 保存DDS失败 %s\n", outPath.string().c_str());
	}
	else
	{
		printf("ConvertExrToSplatMapDDS: %s -> %s 成功\n", 
			   exrInfo.fullPath.string().c_str(), outPath.string().c_str());
	}
}

void NXGUIHoudiniTerrainExporter::ExecuteConvertAll()
{
	std::filesystem::path outputBase(m_nixOutputPath);

	// 转换HeightMap
	if (m_bConvertHeightMap)
	{
		for (const auto& exr : m_heightExrFiles)
		{
			std::string terrainId = std::to_string(exr.tileX) + "_" + std::to_string(exr.tileY);
			std::filesystem::path outDir = outputBase / terrainId;
			std::filesystem::path outPath = outDir / "hmap.dds";
			
			std::filesystem::create_directories(outDir);
			ConvertExrToHeightMapDDS(exr, outPath);
		}
	}

	// 转换SplatMap
	if (m_bConvertSplatMap)
	{
		for (const auto& exr : m_splatExrFiles)
		{
			std::string terrainId = std::to_string(exr.tileX) + "_" + std::to_string(exr.tileY);
			std::filesystem::path outDir = outputBase / terrainId;
			std::filesystem::path outPath = outDir / "splatmap.dds";
			
			std::filesystem::create_directories(outDir);
			ConvertExrToSplatMapDDS(exr, outPath);
		}
	}

	// 刷新DDS列表
	ScanNixDdsFiles();
}

void NXGUIHoudiniTerrainExporter::ComposeHeightMap2DArray()
{
	if (m_nixDdsFiles.empty())
		return;

	auto sortedIndices = GetSortedSliceIndices();
	uint32_t arraySize = static_cast<uint32_t>(sortedIndices.size());

	// 假设所有heightmap尺寸相同，读取第一个获取尺寸
	// TODO: 实际实现需要读取DDS头获取尺寸
	constexpr uint32_t kWidth = 2049;
	constexpr uint32_t kHeight = 2049;
	constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R16_UNORM;
	constexpr uint32_t kBytesPerPixel = sizeof(uint16_t);

	std::unique_ptr<ScratchImage> texArray = std::make_unique<ScratchImage>();
	HRESULT hr = texArray->Initialize2D(kFormat, kWidth, kHeight, arraySize, 1);
	if (FAILED(hr))
	{
		printf("ComposeHeightMap2DArray: Initialize2D 失败\n");
		return;
	}

	for (uint32_t sliceIdx = 0; sliceIdx < arraySize; ++sliceIdx)
	{
		int fileIdx = sortedIndices[sliceIdx];
		if (fileIdx < 0 || fileIdx >= (int)m_nixDdsFiles.size())
			continue;

		const auto& ddsInfo = m_nixDdsFiles[fileIdx];
		
		// 读取单个DDS文件
		TexMetadata meta;
		ScratchImage srcImg;
		hr = LoadFromDDSFile(ddsInfo.heightMapPath.wstring().c_str(), DDS_FLAGS_NONE, &meta, srcImg);
		
		if (SUCCEEDED(hr))
		{
			const Image* src = srcImg.GetImage(0, 0, 0);
			const Image* dst = texArray->GetImage(0, sliceIdx, 0);
			
			if (src && dst && meta.width == kWidth && meta.height == kHeight)
			{
				// 逐行复制，处理可能的行对齐差异
				for (uint32_t y = 0; y < kHeight; ++y)
				{
					const uint8_t* srcRow = src->pixels + y * src->rowPitch;
					uint8_t* dstRow = dst->pixels + y * dst->rowPitch;
					std::memcpy(dstRow, srcRow, kWidth * kBytesPerPixel);
				}
			}
			else
			{
				printf("ComposeHeightMap2DArray: 尺寸不匹配 %s\n", ddsInfo.heightMapPath.string().c_str());
			}
		}
		else
		{
			printf("ComposeHeightMap2DArray: 加载失败 %s\n", ddsInfo.heightMapPath.string().c_str());
			// 填充全黑
			const Image* dst = texArray->GetImage(0, sliceIdx, 0);
			if (dst)
			{
				std::memset(dst->pixels, 0, dst->slicePitch);
			}
		}
	}

	// 保存2DArray
	std::filesystem::path outPath(m_heightArrayPath);
	std::filesystem::create_directories(outPath.parent_path());
	
	hr = SaveToDDSFile(texArray->GetImages(), texArray->GetImageCount(), 
					   texArray->GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ComposeHeightMap2DArray: 保存失败 %s\n", outPath.string().c_str());
	}
	else
	{
		printf("ComposeHeightMap2DArray: 保存成功 %s\n", outPath.string().c_str());
	}
}

void NXGUIHoudiniTerrainExporter::ComposeMinMaxZ2DArray()
{
	if (m_nixDdsFiles.empty())
		return;

	auto sortedIndices = GetSortedSliceIndices();
	uint32_t arraySize = static_cast<uint32_t>(sortedIndices.size());

	constexpr uint32_t kSrcWidth = 2049;
	constexpr uint32_t kSrcHeight = 2049;
	constexpr int kStep = 8;
	constexpr uint32_t kMip0Width = kSrcWidth / kStep;
	constexpr uint32_t kMip0Height = kSrcHeight / kStep;
	constexpr int kMipLevels = 6;

	float kMinHeight = m_heightMapRange[0];
	float kMaxHeight = m_heightMapRange[1];

	auto pImage = std::make_unique<ScratchImage>();
	HRESULT hr = pImage->Initialize2D(DXGI_FORMAT_R32G32_FLOAT, kMip0Width, kMip0Height, arraySize, kMipLevels);
	if (FAILED(hr))
	{
		printf("ComposeMinMaxZ2DArray: Initialize2D 失败\n");
		return;
	}

	struct MinMaxZ { float minVal; float maxVal; };

	for (uint32_t sliceIdx = 0; sliceIdx < arraySize; ++sliceIdx)
	{
		int fileIdx = sortedIndices[sliceIdx];
		if (fileIdx < 0 || fileIdx >= (int)m_nixDdsFiles.size())
			continue;

		const auto& ddsInfo = m_nixDdsFiles[fileIdx];
		
		// 读取heightmap
		TexMetadata meta;
		ScratchImage srcImg;
		std::vector<uint16_t> rawData(kSrcWidth * kSrcHeight, 0);
		
		hr = LoadFromDDSFile(ddsInfo.heightMapPath.wstring().c_str(), DDS_FLAGS_NONE, &meta, srcImg);
		if (SUCCEEDED(hr))
		{
			const Image* src = srcImg.GetImage(0, 0, 0);
			if (src && meta.format == DXGI_FORMAT_R16_UNORM)
			{
				for (uint32_t y = 0; y < kSrcHeight && y < meta.height; ++y)
				{
					const uint16_t* srcRow = reinterpret_cast<const uint16_t*>(src->pixels + y * src->rowPitch);
					for (uint32_t x = 0; x < kSrcWidth && x < meta.width; ++x)
					{
						rawData[y * kSrcWidth + x] = srcRow[x];
					}
				}
			}
		}

		// 计算Mip0的MinMaxZ
		std::vector<MinMaxZ> dataZMip0(kMip0Width * kMip0Height);
		
		for (uint32_t y = 0; y + kStep < kSrcHeight; y += kStep)
		{
			for (uint32_t x = 0; x + kStep < kSrcWidth; x += kStep)
			{
				float minZ = std::numeric_limits<float>::max();
				float maxZ = std::numeric_limits<float>::lowest();

				for (int j = 0; j <= kStep; ++j)
				{
					for (int i = 0; i <= kStep; ++i)
					{
						uint32_t yy = std::min(y + j, kSrcHeight - 1);
						uint32_t xx = std::min(x + i, kSrcWidth - 1);

						uint16_t value = rawData[yy * kSrcWidth + xx];
						float normalizedV = static_cast<float>(value) / 65535.0f;

						minZ = std::min(minZ, normalizedV);
						maxZ = std::max(maxZ, normalizedV);
					}
				}

				// Remap到高度范围
				minZ = minZ * (kMaxHeight - kMinHeight) + kMinHeight;
				maxZ = maxZ * (kMaxHeight - kMinHeight) + kMinHeight;

				dataZMip0[(y / kStep) * kMip0Width + (x / kStep)].minVal = minZ;
				dataZMip0[(y / kStep) * kMip0Width + (x / kStep)].maxVal = maxZ;
			}
		}

		// 计算Mip1-5
		constexpr int kMipStep = 2;
		uint32_t prevW = kMip0Width;
		uint32_t prevH = kMip0Height;
		std::vector<std::vector<MinMaxZ>> dataZMips(5);

		for (int mip = 0; mip < 5; ++mip)
		{
			uint32_t currW = prevW / kMipStep;
			uint32_t currH = prevH / kMipStep;
			dataZMips[mip].resize(currW * currH);

			for (uint32_t y = 0; y < prevH; y += kMipStep)
			{
				for (uint32_t x = 0; x < prevW; x += kMipStep)
				{
					float minZ = std::numeric_limits<float>::max();
					float maxZ = std::numeric_limits<float>::lowest();

					for (uint32_t j = 0; j < kMipStep; ++j)
					{
						for (uint32_t i = 0; i < kMipStep; ++i)
						{
							uint32_t index = (y + j) * prevW + (x + i);
							const MinMaxZ& src = (mip == 0) 
								? dataZMip0[index] 
								: dataZMips[mip - 1][index];

							minZ = std::min(minZ, src.minVal);
							maxZ = std::max(maxZ, src.maxVal);
						}
					}

					dataZMips[mip][(y / kMipStep) * currW + (x / kMipStep)].minVal = minZ;
					dataZMips[mip][(y / kMipStep) * currW + (x / kMipStep)].maxVal = maxZ;
				}
			}

			prevW = currW;
			prevH = currH;
		}

		// 复制到目标纹理
		auto copyLevel = [&](uint32_t mip, const std::vector<MinMaxZ>& src)
		{
			const Image* dst = pImage->GetImage(mip, sliceIdx, 0);
			if (dst)
			{
				MinMaxZ* pDst = reinterpret_cast<MinMaxZ*>(dst->pixels);
				std::memcpy(pDst, src.data(), src.size() * sizeof(MinMaxZ));
			}
		};

		copyLevel(0, dataZMip0);
		for (int mip = 0; mip < 5; ++mip)
			copyLevel(mip + 1, dataZMips[mip]);
	}

	// 保存
	std::filesystem::path outPath(m_minMaxZPath);
	std::filesystem::create_directories(outPath.parent_path());

	hr = SaveToDDSFile(pImage->GetImages(), pImage->GetImageCount(),
					   pImage->GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ComposeMinMaxZ2DArray: 保存失败 %s\n", outPath.string().c_str());
	}
	else
	{
		printf("ComposeMinMaxZ2DArray: 保存成功 %s\n", outPath.string().c_str());
	}
}

void NXGUIHoudiniTerrainExporter::ComposeSplatMap2DArray()
{
	if (m_nixDdsFiles.empty())
		return;

	auto sortedIndices = GetSortedSliceIndices();
	uint32_t arraySize = static_cast<uint32_t>(sortedIndices.size());

	// 假设所有splatmap尺寸相同
	constexpr uint32_t kWidth = 2049;
	constexpr uint32_t kHeight = 2049;
	constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R8_UNORM;
	constexpr uint32_t kBytesPerPixel = sizeof(uint8_t);

	std::unique_ptr<ScratchImage> texArray = std::make_unique<ScratchImage>();
	HRESULT hr = texArray->Initialize2D(kFormat, kWidth, kHeight, arraySize, 1);
	if (FAILED(hr))
	{
		printf("ComposeSplatMap2DArray: Initialize2D 失败\n");
		return;
	}

	for (uint32_t sliceIdx = 0; sliceIdx < arraySize; ++sliceIdx)
	{
		int fileIdx = sortedIndices[sliceIdx];
		if (fileIdx < 0 || fileIdx >= (int)m_nixDdsFiles.size())
			continue;

		const auto& ddsInfo = m_nixDdsFiles[fileIdx];
		
		// 读取单个DDS文件
		TexMetadata meta;
		ScratchImage srcImg;
		hr = LoadFromDDSFile(ddsInfo.splatMapPath.wstring().c_str(), DDS_FLAGS_NONE, &meta, srcImg);
		
		if (SUCCEEDED(hr))
		{
			const Image* src = srcImg.GetImage(0, 0, 0);
			const Image* dst = texArray->GetImage(0, sliceIdx, 0);
			
			if (src && dst && meta.width == kWidth && meta.height == kHeight)
			{
				// 逐行复制，处理可能的行对齐差异
				for (uint32_t y = 0; y < kHeight; ++y)
				{
					const uint8_t* srcRow = src->pixels + y * src->rowPitch;
					uint8_t* dstRow = dst->pixels + y * dst->rowPitch;
					std::memcpy(dstRow, srcRow, kWidth * kBytesPerPixel);
				}
			}
			else
			{
				printf("ComposeSplatMap2DArray: 尺寸不匹配 %s\n", ddsInfo.splatMapPath.string().c_str());
			}
		}
		else
		{
			printf("ComposeSplatMap2DArray: 加载失败 %s\n", ddsInfo.splatMapPath.string().c_str());
			// 填充全黑
			const Image* dst = texArray->GetImage(0, sliceIdx, 0);
			if (dst)
			{
				std::memset(dst->pixels, 0, dst->slicePitch);
			}
		}
	}

	// 保存2DArray
	std::filesystem::path outPath(m_splatArrayPath);
	std::filesystem::create_directories(outPath.parent_path());
	
	hr = SaveToDDSFile(texArray->GetImages(), texArray->GetImageCount(), 
					   texArray->GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ComposeSplatMap2DArray: 保存失败 %s\n", outPath.string().c_str());
	}
	else
	{
		printf("ComposeSplatMap2DArray: 保存成功 %s\n", outPath.string().c_str());
	}
}

std::vector<int> NXGUIHoudiniTerrainExporter::GetSortedSliceIndices() const
{
	if (m_nixDdsFiles.empty())
		return {};

	// 找到坐标范围
	int minX = INT_MAX, maxX = INT_MIN;
	int minY = INT_MAX, maxY = INT_MIN;
	
	for (const auto& dds : m_nixDdsFiles)
	{
		minX = std::min(minX, dds.tileX);
		maxX = std::max(maxX, dds.tileX);
		minY = std::min(minY, dds.tileY);
		maxY = std::max(maxY, dds.tileY);
	}

	int countX = maxX - minX + 1;
	int countY = maxY - minY + 1;

	// 建立坐标到文件索引的映射
	std::map<std::pair<int, int>, int> coordToIdx;
	for (int i = 0; i < (int)m_nixDdsFiles.size(); ++i)
	{
		coordToIdx[{m_nixDdsFiles[i].tileX, m_nixDdsFiles[i].tileY}] = i;
	}

	// 根据排序设置生成slice顺序
	std::vector<int> result;
	result.reserve(m_nixDdsFiles.size());

	if (m_bColumnFirst)
	{
		// 列优先：外层遍历列(X)，内层遍历行(Y)
		for (int colIdx = 0; colIdx < countX; ++colIdx)
		{
			int x = m_bColAscending ? (minX + colIdx) : (maxX - colIdx);
			
			for (int rowIdx = 0; rowIdx < countY; ++rowIdx)
			{
				int y = m_bRowAscending ? (minY + rowIdx) : (maxY - rowIdx);
				
				auto it = coordToIdx.find({x, y});
				if (it != coordToIdx.end())
				{
					result.push_back(it->second);
				}
			}
		}
	}
	else
	{
		// 行优先：外层遍历行(Y)，内层遍历列(X)
		for (int rowIdx = 0; rowIdx < countY; ++rowIdx)
		{
			int y = m_bRowAscending ? (minY + rowIdx) : (maxY - rowIdx);
			
			for (int colIdx = 0; colIdx < countX; ++colIdx)
			{
				int x = m_bColAscending ? (minX + colIdx) : (maxX - colIdx);
				
				auto it = coordToIdx.find({x, y});
				if (it != coordToIdx.end())
				{
					result.push_back(it->second);
				}
			}
		}
	}

	return result;
}

bool NXGUIHoudiniTerrainExporter::ParseTileCoord(const std::string& fileName, int& outX, int& outY)
{
	// 解析格式: "X_Y.exr" 或 "X_Y"
	std::regex pattern(R"((-?\d+)_(-?\d+))");
	std::smatch match;
	
	if (std::regex_search(fileName, match, pattern))
	{
		outX = std::stoi(match[1].str());
		outY = std::stoi(match[2].str());
		return true;
	}
	
	return false;
}
