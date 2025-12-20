#pragma once
#include "BaseDefs/DearImGui.h"
#include "BaseDefs/NixCore.h"
#include "NXTexture.h"
#include <filesystem>
#include <vector>
#include <string>

// 单个Slice的材质信息
struct TerrainMaterialSlice
{
	char albedoPath[512] = "";
	char normalPath[512] = "";
	char roughnessPath[512] = "";
	char specularPath[512] = "";
	char aoPath[512] = "";
	
	// 用于文件夹快捷匹配
	char folderPath[512] = "";
};

class NXGUITerrainMaterialGenerator
{
public:
	NXGUITerrainMaterialGenerator();
	virtual ~NXGUITerrainMaterialGenerator();

	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	// GUI渲染
	void RenderSliceListWithPreview();
	void RenderTexturePreviewRow(int sliceIndex, float previewSize);
	void RenderComposeSection();

	// 文件夹快捷匹配
	void AutoMatchFromFolder(int sliceIndex);
	void AutoMatchAllSlices();

	// 清空Slice路径
	void ClearSlice(int sliceIndex);
	void ClearAllSlices();

	// 导出/导入配置
	void ExportConfig();
	void ImportConfig();

	// 合成2DArray
	void ComposeAlbedo2DArray();
	void ComposeNormal2DArray();
	void ComposeRoughness2DArray();
	void ComposeSpecular2DArray();
	void ComposeAO2DArray();

	// 通用合成函数
	void ComposeTexture2DArray(
		const std::vector<std::string>& texturePaths,
		const std::filesystem::path& outPath,
		bool useBC7Compression,
		bool useSRGB,
		const char* textureName);

	// 检查字符串是否包含子串(不区分大小写)
	bool ContainsIgnoreCase(const std::string& str, const std::string& substr);

	// 获取文件名(不包含路径)
	std::string GetFileName(const std::string& path);

private:
	bool m_bVisible = false;

	// Slice数组（固定256个元素，使用m_sliceCount控制实际数量）
	std::vector<TerrainMaterialSlice> m_slices;
	int m_sliceCount = 1;
	int m_selectedSliceIndex = 0;

	// 输出路径
	char m_albedoArrayPath[512] = "D:\\NixAssets\\Terrain\\terrainAlbedo2DArray.dds";
	char m_normalArrayPath[512] = "D:\\NixAssets\\Terrain\\terrainNormal2DArray.dds";
	char m_roughnessArrayPath[512] = "D:\\NixAssets\\Terrain\\terrainRoughness2DArray.dds";
	char m_specularArrayPath[512] = "D:\\NixAssets\\Terrain\\terrainSpecular2DArray.dds";
	char m_aoArrayPath[512] = "D:\\NixAssets\\Terrain\\terrainAO2DArray.dds";

	// 合成选项
	bool m_bComposeAlbedo = true;
	bool m_bComposeNormal = true;
	bool m_bComposeRoughness = false;
	bool m_bComposeSpecular = false;
	bool m_bComposeAO = false;

	// 输出纹理尺寸
	int m_outputTextureSizeIndex = 2; // 0=256, 1=512, 2=1024, 3=2048
	int m_outputTextureSize = 1024;

	// 配置文件路径
	char m_configFilePath[512] = "D:\\NixAssets\\Terrain\\terrainTextureConfig.txt";
};
