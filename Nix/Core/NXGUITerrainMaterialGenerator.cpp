#include "NXGUITerrainMaterialGenerator.h"
#include "NXSerializable.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXAllocatorManager.h"
#include "NXConverter.h"
#include <DirectXTex.h>
#include <algorithm>
#include <cctype>

using namespace DirectX;

NXGUITerrainMaterialGenerator::NXGUITerrainMaterialGenerator()
{
	// 固定初始化256个slice，使用m_sliceCount控制实际显示数量
	m_slices.resize(256);
	m_sliceCount = 1;
}

NXGUITerrainMaterialGenerator::~NXGUITerrainMaterialGenerator()
{
}

void NXGUITerrainMaterialGenerator::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(1400.0f, 700.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("地形材质纹理生成器"), &m_bVisible))
	{
		// 上部分: 合成区域
		RenderComposeSection();

		ImGui::Separator();
		ImGui::Spacing();

		// 下部分: Slice列表（每行左侧文件夹路径+匹配按钮，右侧纹理预览）
		RenderSliceListWithPreview();
	}
	ImGui::End();
}

void NXGUITerrainMaterialGenerator::RenderSliceListWithPreview()
{
	ImGui::Text(ImUtf8("=== Slice 列表 ==="));
	
	// Slice数量控制
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::InputInt(ImUtf8("Slice数量"), &m_sliceCount))
	{
		if (m_sliceCount < 1) m_sliceCount = 1;
		if (m_sliceCount > 256) m_sliceCount = 256;
	}

	ImGui::SameLine();
	if (ImGui::Button(ImUtf8("清空所有路径")))
	{
		ClearAllSlices();
	}

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.2f, 0.0f, 1.0f));
	if (ImGui::Button(ImUtf8("匹配全部纹理（可能比较久）")))
	{
		AutoMatchAllSlices();
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	// 导出/导入配置按钮
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.5f, 1.0f));
	if (ImGui::Button(ImUtf8("导出配置")))
	{
		ExportConfig();
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.4f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.5f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.3f, 0.1f, 1.0f));
	if (ImGui::Button(ImUtf8("导入配置")))
	{
		ImportConfig();
	}
	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 滚动区域显示所有slice
	float previewSize = 64.0f;
	float spacing = 5.0f;
	float framePadding = ImGui::GetStyle().FramePadding.x; // ImageButton 左右各有 frame padding
	float buttonWidth = previewSize + framePadding * 2;    // 单个按钮实际宽度
	float rightColumnWidth = buttonWidth * 5 + spacing * 4 + 10.0f; // 5个按钮 + 4个间距 + 额外边距
	
	ImGui::BeginChild("SliceScrollArea", ImVec2(0, 0), false);
	
	for (int i = 0; i < m_sliceCount; i++)
	{
		TerrainMaterialSlice& slice = m_slices[i];
		
		ImGui::PushID(i);
		
		// 使用不可见的表格布局，左侧放置路径/按钮，右侧放置图像
		if (ImGui::BeginTable("##SliceTable", 2, ImGuiTableFlags_SizingStretchProp))
		{
			// 左侧列占用剩余空间
			ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch);
			// 右侧列固定宽度（5张图片 + 间距 + frame padding）
			ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
			
			ImGui::TableNextRow();
			
			// 左侧列：路径、匹配按钮、清空按钮
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginGroup();
			
			// 第一行：Slice标签 + 文件夹路径
			ImGui::Text("Slice %d:", i);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##FolderPath", slice.folderPath, sizeof(slice.folderPath));
			
			// 第二行：匹配按钮和清空按钮
			if (ImGui::Button(ImUtf8("匹配")))
			{
				AutoMatchFromFolder(i);
			}
			ImGui::SameLine();
			if (ImGui::Button(ImUtf8("清空")))
			{
				ClearSlice(i);
			}
			
			ImGui::EndGroup();
			
			// 右侧列：5张纹理预览
			ImGui::TableSetColumnIndex(1);
			RenderTexturePreviewRow(i, previewSize);
			
			ImGui::EndTable();
		}
		
		ImGui::PopID();
		
		ImGui::Spacing();
		if (i < m_sliceCount - 1)
		{
			ImGui::Separator();
			ImGui::Spacing();
		}
	}
	
	ImGui::EndChild();
}

void NXGUITerrainMaterialGenerator::RenderTexturePreviewRow(int sliceIndex, float previewSize)
{
	if (sliceIndex < 0 || sliceIndex >= m_sliceCount)
		return;

	TerrainMaterialSlice& slice = m_slices[sliceIndex];
	float spacing = 5.0f;

	const char* labels[] = { "A", "N", "R", "S", "O" };
	char* pathPtrs[] = { slice.albedoPath, slice.normalPath, slice.roughnessPath, slice.specularPath, slice.aoPath };

	for (int i = 0; i < 5; i++)
	{
		ImGui::PushID(i);

		// 获取纹理：如果路径有效则加载纹理，否则使用默认白色纹理
		std::string texPath = pathPtrs[i];
		Ntr<NXTexture2D> pTex;
		if (!texPath.empty() && std::filesystem::exists(texPath))
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("", texPath);
		if (pTex.IsNull())
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);

		// 使用 ImageButton 显示纹理预览
		ImVec2 size = ImVec2(previewSize, previewSize);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 bg_col = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		NXShVisDescHeap->PushFluid(pTex->GetSRV());
		auto& srvHandle = NXShVisDescHeap->Submit();

		std::string btnId = std::string("##TexBtn_") + labels[i];
		if (ImGui::ImageButton(btnId.c_str(), (ImTextureID)srvHandle.ptr, size, uv0, uv1, bg_col, tint_col))
		{
			// 点击按钮时的操作（可扩展：打开文件选择器等）
		}

		// 检测 Win32 文件拖放
		std::filesystem::path droppedFile = NXGUICommon::AcceptWinFileDropImage();
		if (!droppedFile.empty())
		{
			strncpy_s(pathPtrs[i], 512, droppedFile.string().c_str(), _TRUNCATE);
			printf("Slice %d - %s: 已接收拖入文件 %s\n", sliceIndex, labels[i], droppedFile.string().c_str());
		}

		// 在图片右下角显示标签
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 rectMin = ImGui::GetItemRectMin();
		ImVec2 rectMax = ImGui::GetItemRectMax();
		ImVec2 textSize = ImGui::CalcTextSize(labels[i]);
		float labelPadding = 2.0f;
		ImVec2 labelPos = ImVec2(rectMax.x - textSize.x - labelPadding, rectMax.y - textSize.y - labelPadding);
		
		// 绘制标签背景
		drawList->AddRectFilled(
			ImVec2(labelPos.x - 2, labelPos.y - 1),
			ImVec2(rectMax.x, rectMax.y),
			IM_COL32(0, 0, 0, 180));
		drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), labels[i]);

		ImGui::PopID();

		if (i < 4)
			ImGui::SameLine(0, spacing);
	}
}

void NXGUITerrainMaterialGenerator::RenderComposeSection()
{
	ImGui::Text(ImUtf8("=== 合成 2D Array ==="));
	ImGui::Spacing();

	// 输出纹理尺寸下拉菜单
	const char* textureSizes[] = { "256", "512", "1024", "2048" };
	const int textureSizeValues[] = { 256, 512, 1024, 2048 };
	
	ImGui::Text(ImUtf8("输出纹理尺寸:"));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::BeginCombo("##TextureSize", textureSizes[m_outputTextureSizeIndex]))
	{
		for (int i = 0; i < 4; i++)
		{
			bool isSelected = (m_outputTextureSizeIndex == i);
			if (ImGui::Selectable(textureSizes[i], isSelected))
			{
				m_outputTextureSizeIndex = i;
				m_outputTextureSize = textureSizeValues[i];
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
    
	ImGui::SameLine();

	// 合成选项 - 横向排列
	ImGui::Checkbox("Albedo", &m_bComposeAlbedo);
	ImGui::SameLine();
	ImGui::Checkbox("Normal", &m_bComposeNormal);
	ImGui::SameLine();
	ImGui::Checkbox("Roughness", &m_bComposeRoughness);
	ImGui::SameLine();
	ImGui::Checkbox("Specular", &m_bComposeSpecular);
	ImGui::SameLine();
	ImGui::Checkbox("AO", &m_bComposeAO);

	ImGui::Spacing();

	// 输出路径
	ImGui::Text(ImUtf8("输出路径:"));
	
	if (m_bComposeAlbedo)
	{
		ImGui::Text("Albedo: ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##AlbedoPath", m_albedoArrayPath, sizeof(m_albedoArrayPath));
	}
	
	if (m_bComposeNormal)
	{
		ImGui::Text("Normal: ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##NormalPath", m_normalArrayPath, sizeof(m_normalArrayPath));
	}

	if (m_bComposeRoughness)
	{
		ImGui::Text("Roughness:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##RoughnessPath", m_roughnessArrayPath, sizeof(m_roughnessArrayPath));
	}

	if (m_bComposeSpecular)
	{
		ImGui::Text("Specular:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##SpecularPath", m_specularArrayPath, sizeof(m_specularArrayPath));
	}

	if (m_bComposeAO)
	{
		ImGui::Text("AO:     ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##AOPath", m_aoArrayPath, sizeof(m_aoArrayPath));
	}

	ImGui::Spacing();

	// 醒目的合成按钮
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
	
	bool canCompose = !m_slices.empty() && (m_bComposeAlbedo || m_bComposeNormal || m_bComposeRoughness || m_bComposeSpecular || m_bComposeAO);
	if (!canCompose)
		ImGui::BeginDisabled();

	if (ImGui::Button(ImUtf8(">>> 开始合成 <<<"), ImVec2(200, 40)))
	{
		if (m_bComposeAlbedo)
			ComposeAlbedo2DArray();
		if (m_bComposeNormal)
			ComposeNormal2DArray();
		if (m_bComposeRoughness)
			ComposeRoughness2DArray();
		if (m_bComposeSpecular)
			ComposeSpecular2DArray();
		if (m_bComposeAO)
			ComposeAO2DArray();
	}

	if (!canCompose)
		ImGui::EndDisabled();

	ImGui::PopStyleColor(3);
}

void NXGUITerrainMaterialGenerator::AutoMatchFromFolder(int sliceIndex)
{
	if (sliceIndex < 0 || sliceIndex >= m_sliceCount)
		return;

	TerrainMaterialSlice& slice = m_slices[sliceIndex];
	std::filesystem::path folderPath(slice.folderPath);

	if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath))
	{
		printf("AutoMatchFromFolder: 文件夹不存在或无效 %s\n", slice.folderPath);
		return;
	}

	// 清空当前路径
	slice.albedoPath[0] = '\0';
	slice.normalPath[0] = '\0';
	slice.roughnessPath[0] = '\0';
	slice.specularPath[0] = '\0';
	slice.aoPath[0] = '\0';

	// 遍历文件夹中的文件
	for (const auto& entry : std::filesystem::directory_iterator(folderPath))
	{
		if (!entry.is_regular_file())
			continue;

		std::string fileName = entry.path().filename().string();
		std::string fullPath = entry.path().string();

		// 检查是否是图片文件
		std::string ext = entry.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".tga" && ext != ".dds" && ext != ".exr" && ext != ".bmp")
			continue;

		// 匹配各类型纹理
		if (ContainsIgnoreCase(fileName, "basecolor") || ContainsIgnoreCase(fileName, "albedo") || ContainsIgnoreCase(fileName, "diffuse"))
		{
			strncpy_s(slice.albedoPath, fullPath.c_str(), sizeof(slice.albedoPath) - 1);
		}
		else if (ContainsIgnoreCase(fileName, "normal"))
		{
			strncpy_s(slice.normalPath, fullPath.c_str(), sizeof(slice.normalPath) - 1);
		}
		else if (ContainsIgnoreCase(fileName, "roughness") || ContainsIgnoreCase(fileName, "rough"))
		{
			strncpy_s(slice.roughnessPath, fullPath.c_str(), sizeof(slice.roughnessPath) - 1);
		}
		else if (ContainsIgnoreCase(fileName, "specular") || ContainsIgnoreCase(fileName, "spec"))
		{
			strncpy_s(slice.specularPath, fullPath.c_str(), sizeof(slice.specularPath) - 1);
		}
		else if (ContainsIgnoreCase(fileName, "ao") || ContainsIgnoreCase(fileName, "ambientocclusion") || ContainsIgnoreCase(fileName, "occlusion"))
		{
			strncpy_s(slice.aoPath, fullPath.c_str(), sizeof(slice.aoPath) - 1);
		}
	}

	printf("AutoMatchFromFolder: 从 %s 匹配完成\n", slice.folderPath);
}

void NXGUITerrainMaterialGenerator::AutoMatchAllSlices()
{
	printf("AutoMatchAllSlices: 开始匹配全部 %d 个slice...\n", m_sliceCount);
	for (int i = 0; i < m_sliceCount; i++)
	{
		AutoMatchFromFolder(i);
	}
	printf("AutoMatchAllSlices: 全部匹配完成\n");
}

void NXGUITerrainMaterialGenerator::ClearSlice(int sliceIndex)
{
	if (sliceIndex < 0 || sliceIndex >= 256)
		return;

	TerrainMaterialSlice& slice = m_slices[sliceIndex];
	slice.folderPath[0] = '\0';
	slice.albedoPath[0] = '\0';
	slice.normalPath[0] = '\0';
	slice.roughnessPath[0] = '\0';
	slice.specularPath[0] = '\0';
	slice.aoPath[0] = '\0';
}

void NXGUITerrainMaterialGenerator::ClearAllSlices()
{
	for (int i = 0; i < 256; i++)
	{
		ClearSlice(i);
	}
}

void NXGUITerrainMaterialGenerator::ComposeAlbedo2DArray()
{
	std::vector<std::string> paths;
	for (int i = 0; i < m_sliceCount; i++)
	{
		paths.push_back(m_slices[i].albedoPath);
	}
	ComposeTexture2DArray(paths, std::filesystem::path(m_albedoArrayPath), true, true, true, "Albedo");
}

void NXGUITerrainMaterialGenerator::ComposeNormal2DArray()
{
	std::vector<std::string> paths;
	for (int i = 0; i < m_sliceCount; i++)
	{
		paths.push_back(m_slices[i].normalPath);
	}
	ComposeTexture2DArray(paths, std::filesystem::path(m_normalArrayPath), true, false, false, "Normal");
}

void NXGUITerrainMaterialGenerator::ComposeRoughness2DArray()
{
	std::vector<std::string> paths;
	for (int i = 0; i < m_sliceCount; i++)
	{
		paths.push_back(m_slices[i].roughnessPath);
	}
	ComposeTexture2DArray(paths, std::filesystem::path(m_roughnessArrayPath), true, false, false, "Roughness");
}

void NXGUITerrainMaterialGenerator::ComposeSpecular2DArray()
{
	std::vector<std::string> paths;
	for (int i = 0; i < m_sliceCount; i++)
	{
		paths.push_back(m_slices[i].specularPath);
	}
	ComposeTexture2DArray(paths, std::filesystem::path(m_specularArrayPath), true, false, false, "Specular");
}

void NXGUITerrainMaterialGenerator::ComposeAO2DArray()
{
	std::vector<std::string> paths;
	for (int i = 0; i < m_sliceCount; i++)
	{
		paths.push_back(m_slices[i].aoPath);
	}
	ComposeTexture2DArray(paths, std::filesystem::path(m_aoArrayPath), true, false, false, "AO");
}

void NXGUITerrainMaterialGenerator::ComposeTexture2DArray(
	const std::vector<std::string>& texturePaths,
	const std::filesystem::path& outPath,
	bool useBC7Compression,
    bool inputIsSRGB,
	bool outputUseSRGB,
	const char* textureName)
{
	if (texturePaths.empty())
	{
		printf("ComposeTexture2DArray: %s 路径列表为空\n", textureName);
		return;
	}

	uint32_t arraySize = static_cast<uint32_t>(texturePaths.size());
	uint32_t width = static_cast<uint32_t>(m_outputTextureSize);
	uint32_t height = static_cast<uint32_t>(m_outputTextureSize);

	// 计算mip层级数
	uint32_t mipLevels = 1;
	uint32_t tempSize = std::max(width, height);
	while (tempSize > 1)
	{
		tempSize >>= 1;
		mipLevels++;
	}

	// 创建临时的RGBA8格式纹理数组（仅mip0）
	std::unique_ptr<ScratchImage> texArray = std::make_unique<ScratchImage>();
	HRESULT hr = texArray->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, arraySize, 1);
	if (FAILED(hr))
	{
		printf("ComposeTexture2DArray: %s Initialize2D 失败\n", textureName);
		return;
	}

	// 填充每个slice
	for (uint32_t sliceIdx = 0; sliceIdx < arraySize; ++sliceIdx)
	{
		const Image* dstImage = texArray->GetImage(0, sliceIdx, 0);
		if (!dstImage)
		{
			printf("ComposeTexture2DArray: %s 获取slice %u 失败\n", textureName, sliceIdx);
			continue;
		}

		const std::string& srcPath = texturePaths[sliceIdx];
		if (srcPath.empty())
		{
			// 路径为空，填充默认颜色 (灰色)
			memset(dstImage->pixels, 128, dstImage->rowPitch * height);
			printf("ComposeTexture2DArray: %s slice %u 路径为空，填充默认颜色\n", textureName, sliceIdx);
			continue;
		}

		// 加载源纹理
		std::filesystem::path srcFilePath(srcPath);
		if (!std::filesystem::exists(srcFilePath))
		{
			memset(dstImage->pixels, 128, dstImage->rowPitch * height);
			printf("ComposeTexture2DArray: %s slice %u 文件不存在 %s\n", textureName, sliceIdx, srcPath.c_str());
			continue;
		}

		ScratchImage srcImage;
		std::wstring wSrcPath = srcFilePath.wstring();
		std::string ext = srcFilePath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == ".dds")
		{
			hr = LoadFromDDSFile(wSrcPath.c_str(), DDS_FLAGS_NONE, nullptr, srcImage);
		}
		else if (ext == ".tga")
		{
			hr = LoadFromTGAFile(wSrcPath.c_str(), nullptr, srcImage);
		}
		else if (ext == ".hdr")
		{
			hr = LoadFromHDRFile(wSrcPath.c_str(), nullptr, srcImage);
		}
		else
		{
			WIC_FLAGS wicFlags = inputIsSRGB ? WIC_FLAGS_NONE : WIC_FLAGS_IGNORE_SRGB;
			hr = LoadFromWICFile(wSrcPath.c_str(), wicFlags, nullptr, srcImage);
		}

		if (FAILED(hr))
		{
			memset(dstImage->pixels, 128, dstImage->rowPitch * height);
			printf("ComposeTexture2DArray: %s slice %u 加载失败 %s\n", textureName, sliceIdx, srcPath.c_str());
			continue;
		}

		// 转换为RGBA8格式（如果需要）
		ScratchImage convertedImage;
		const Image* pSrcImage = srcImage.GetImage(0, 0, 0);
		if (pSrcImage->format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			hr = Convert(*pSrcImage, DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, convertedImage);
			if (SUCCEEDED(hr))
			{
				pSrcImage = convertedImage.GetImage(0, 0, 0);
			}
		}

		// 缩放到目标尺寸（如果需要）
		ScratchImage resizedImage;
		if (pSrcImage->width != width || pSrcImage->height != height)
		{
			hr = Resize(*pSrcImage, width, height, TEX_FILTER_LINEAR, resizedImage);
			if (SUCCEEDED(hr))
			{
				pSrcImage = resizedImage.GetImage(0, 0, 0);
			}
			else
			{
				memset(dstImage->pixels, 128, dstImage->rowPitch * height);
				printf("ComposeTexture2DArray: %s slice %u 缩放失败\n", textureName, sliceIdx);
				continue;
			}
		}

		// 复制到目标
		size_t copyRowPitch = std::min(pSrcImage->rowPitch, dstImage->rowPitch);
		for (uint32_t y = 0; y < height; ++y)
		{
			const uint8_t* srcRow = pSrcImage->pixels + y * pSrcImage->rowPitch;
			uint8_t* dstRow = dstImage->pixels + y * dstImage->rowPitch;
			memcpy(dstRow, srcRow, copyRowPitch);
		}

		printf("ComposeTexture2DArray: %s slice %u 加载成功 %s\n", textureName, sliceIdx, srcPath.c_str());
	}

	// 生成mipmap
	std::unique_ptr<ScratchImage> texArrayWithMips = std::make_unique<ScratchImage>();
	hr = GenerateMipMaps(texArray->GetImages(), texArray->GetImageCount(), texArray->GetMetadata(),
						 TEX_FILTER_LINEAR, 0, *texArrayWithMips);
	if (FAILED(hr))
	{
		printf("ComposeTexture2DArray: %s mipmap生成失败，使用无mip版本\n", textureName);
		texArrayWithMips = std::move(texArray);
	}
	else
	{
		printf("ComposeTexture2DArray: %s mipmap生成成功 (%u levels)\n", textureName, (uint32_t)texArrayWithMips->GetMetadata().mipLevels);
	}

	// 压缩为BC7格式（如果需要）
	std::unique_ptr<ScratchImage> finalImage;
	if (useBC7Compression)
	{
		DXGI_FORMAT compressFormat = outputUseSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
		finalImage = std::make_unique<ScratchImage>();
		hr = Compress(texArrayWithMips->GetImages(), texArrayWithMips->GetImageCount(), texArrayWithMips->GetMetadata(),
					  compressFormat, TEX_COMPRESS_BC7_QUICK | TEX_COMPRESS_PARALLEL, TEX_THRESHOLD_DEFAULT, *finalImage);
		if (FAILED(hr))
		{
			printf("ComposeTexture2DArray: %s BC7压缩失败，使用未压缩格式\n", textureName);
			finalImage = std::move(texArrayWithMips);
		}
		else
		{
			printf("ComposeTexture2DArray: %s BC7压缩成功 (SRGB=%s)\n", textureName, outputUseSRGB ? "true" : "false");
		}
	}
	else
	{
		finalImage = std::move(texArrayWithMips);
	}

	// 保存DDS
	std::filesystem::create_directories(outPath.parent_path());
	hr = SaveToDDSFile(finalImage->GetImages(), finalImage->GetImageCount(),
					   finalImage->GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
	if (FAILED(hr))
	{
		printf("ComposeTexture2DArray: %s 保存DDS失败 %s\n", textureName, outPath.string().c_str());
	}
	else
	{
		printf("ComposeTexture2DArray: %s 保存成功 %s (%ux%ux%u)\n", 
			   textureName, outPath.string().c_str(), width, height, arraySize);
	}
}

bool NXGUITerrainMaterialGenerator::ContainsIgnoreCase(const std::string& str, const std::string& substr)
{
	std::string strLower = str;
	std::string substrLower = substr;
	std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
	std::transform(substrLower.begin(), substrLower.end(), substrLower.begin(), ::tolower);
	return strLower.find(substrLower) != std::string::npos;
}

std::string NXGUITerrainMaterialGenerator::GetFileName(const std::string& path)
{
	std::filesystem::path p(path);
	return p.filename().string();
}

void NXGUITerrainMaterialGenerator::ExportConfig()
{
	const std::filesystem::path configPath(m_configFilePath);
	
	// 确保目录存在
	std::filesystem::create_directories(configPath.parent_path());

	NXSerializer serializer;
	serializer.StartObject();

	// 保存slice数量
	serializer.Int("sliceCount", m_sliceCount);

	// 保存输出纹理尺寸索引
	serializer.Int("outputTextureSizeIndex", m_outputTextureSizeIndex);
	serializer.Int("outputTextureSize", m_outputTextureSize);

	// 保存合成选项
	serializer.Bool("composeAlbedo", m_bComposeAlbedo);
	serializer.Bool("composeNormal", m_bComposeNormal);
	serializer.Bool("composeRoughness", m_bComposeRoughness);
	serializer.Bool("composeSpecular", m_bComposeSpecular);
	serializer.Bool("composeAO", m_bComposeAO);

	// 保存输出路径
	serializer.String("albedoArrayPath", m_albedoArrayPath);
	serializer.String("normalArrayPath", m_normalArrayPath);
	serializer.String("roughnessArrayPath", m_roughnessArrayPath);
	serializer.String("specularArrayPath", m_specularArrayPath);
	serializer.String("aoArrayPath", m_aoArrayPath);

	// 保存所有slice数据
	serializer.StartArray("slices");
	for (int i = 0; i < m_sliceCount; i++)
	{
		const TerrainMaterialSlice& slice = m_slices[i];
		serializer.StartObject();
		serializer.String("folderPath", slice.folderPath);
		serializer.String("albedoPath", slice.albedoPath);
		serializer.String("normalPath", slice.normalPath);
		serializer.String("roughnessPath", slice.roughnessPath);
		serializer.String("specularPath", slice.specularPath);
		serializer.String("aoPath", slice.aoPath);
		serializer.EndObject();
	}
	serializer.EndArray();

	serializer.EndObject();

	serializer.SaveToFile(configPath);
	printf("ExportConfig: 配置已导出到 %s\n", configPath.string().c_str());
}

void NXGUITerrainMaterialGenerator::ImportConfig()
{
	const std::filesystem::path configPath(m_configFilePath);

	if (!std::filesystem::exists(configPath))
	{
		printf("ImportConfig: 配置文件不存在 %s\n", configPath.string().c_str());
		return;
	}

	NXDeserializer deserializer;
	if (!deserializer.LoadFromFile(configPath))
	{
		printf("ImportConfig: 配置文件加载失败 %s\n", configPath.string().c_str());
		return;
	}

	// 读取slice数量
	m_sliceCount = deserializer.Int("sliceCount", 1);
	if (m_sliceCount < 1) m_sliceCount = 1;
	if (m_sliceCount > 256) m_sliceCount = 256;

	// 读取输出纹理尺寸索引
	m_outputTextureSizeIndex = deserializer.Int("outputTextureSizeIndex", 2);
	m_outputTextureSize = deserializer.Int("outputTextureSize", 1024);

	// 读取合成选项
	m_bComposeAlbedo = deserializer.Bool("composeAlbedo", true);
	m_bComposeNormal = deserializer.Bool("composeNormal", true);
	m_bComposeRoughness = deserializer.Bool("composeRoughness", true);
	m_bComposeSpecular = deserializer.Bool("composeSpecular", true);
	m_bComposeAO = deserializer.Bool("composeAO", true);

	// 读取输出路径
	std::string albedoPath = deserializer.String("albedoArrayPath", "D:\\NixAssets\\Terrain\\terrainAlbedo2DArray.dds");
	std::string normalPath = deserializer.String("normalArrayPath", "D:\\NixAssets\\Terrain\\terrainNormal2DArray.dds");
	std::string roughnessPath = deserializer.String("roughnessArrayPath", "D:\\NixAssets\\Terrain\\terrainRoughness2DArray.dds");
	std::string specularPath = deserializer.String("specularArrayPath", "D:\\NixAssets\\Terrain\\terrainSpecular2DArray.dds");
	std::string aoPath = deserializer.String("aoArrayPath", "D:\\NixAssets\\Terrain\\terrainAO2DArray.dds");

	strncpy_s(m_albedoArrayPath, albedoPath.c_str(), sizeof(m_albedoArrayPath) - 1);
	strncpy_s(m_normalArrayPath, normalPath.c_str(), sizeof(m_normalArrayPath) - 1);
	strncpy_s(m_roughnessArrayPath, roughnessPath.c_str(), sizeof(m_roughnessArrayPath) - 1);
	strncpy_s(m_specularArrayPath, specularPath.c_str(), sizeof(m_specularArrayPath) - 1);
	strncpy_s(m_aoArrayPath, aoPath.c_str(), sizeof(m_aoArrayPath) - 1);

	// 清空所有slice
	ClearAllSlices();

	// 读取所有slice数据
	auto slicesArray = deserializer.Array("slices");
	int sliceIndex = 0;
	for (const auto& sliceVal : slicesArray)
	{
		if (sliceIndex >= m_sliceCount)
			break;

		TerrainMaterialSlice& slice = m_slices[sliceIndex];
		
		std::string folderPath = deserializer.String(sliceVal, "folderPath", "");
		std::string albedo = deserializer.String(sliceVal, "albedoPath", "");
		std::string normal = deserializer.String(sliceVal, "normalPath", "");
		std::string roughness = deserializer.String(sliceVal, "roughnessPath", "");
		std::string specular = deserializer.String(sliceVal, "specularPath", "");
		std::string ao = deserializer.String(sliceVal, "aoPath", "");

		strncpy_s(slice.folderPath, folderPath.c_str(), sizeof(slice.folderPath) - 1);
		strncpy_s(slice.albedoPath, albedo.c_str(), sizeof(slice.albedoPath) - 1);
		strncpy_s(slice.normalPath, normal.c_str(), sizeof(slice.normalPath) - 1);
		strncpy_s(slice.roughnessPath, roughness.c_str(), sizeof(slice.roughnessPath) - 1);
		strncpy_s(slice.specularPath, specular.c_str(), sizeof(slice.specularPath) - 1);
		strncpy_s(slice.aoPath, ao.c_str(), sizeof(slice.aoPath) - 1);

		sliceIndex++;
	}

	printf("ImportConfig: 配置已从 %s 导入，共 %d 个slice\n", configPath.string().c_str(), m_sliceCount);
}
