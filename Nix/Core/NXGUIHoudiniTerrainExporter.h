#pragma once
#include "BaseDefs/DearImGui.h"
#include <filesystem>
#include <vector>
#include <string>
#include <map>

// Houdini EXR文件信息
struct HoudiniExrFileInfo
{
	std::filesystem::path fullPath;
	std::string fileName;	// 如 "0_0.exr"
	int tileX = 0;			// 从文件名解析的X坐标
	int tileY = 0;			// 从文件名解析的Y坐标
};

// Nix DDS文件信息  
struct NixTerrainDdsInfo
{
	std::filesystem::path heightMapPath;	// hmap.dds路径
	std::filesystem::path splatMapPath;		// splatmap.dds路径
	std::filesystem::path normalMapPath;	// nmap.dds路径
	std::string terrainId;					// 如 "0_0"
	int tileX = 0;
	int tileY = 0;
};

class NXGUIHoudiniTerrainExporter
{
public:
	NXGUIHoudiniTerrainExporter();
	virtual ~NXGUIHoudiniTerrainExporter();

	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	// GUI渲染各列
	void RenderColumn1_HoudiniFiles();
	void RenderColumn2_ConvertToDDS();
	void RenderColumn3_BakeSubTiles();
	void RenderColumn4_Compose2DArray();

	// 扫描Houdini EXR文件
	void ScanHoudiniExrFiles();
	
	// 扫描Nix DDS文件
	void ScanNixDdsFiles();

	// 转换单个EXR到DDS (heightmap)
	void ConvertExrToHeightMapDDS(const HoudiniExrFileInfo& exrInfo, const std::filesystem::path& outPath);
	
	// 转换单个EXR到DDS (splatmap)
	void ConvertExrToSplatMapDDS(const HoudiniExrFileInfo& exrInfo, const std::filesystem::path& outPath);
	
	// 转换单个EXR到DDS (normalmap)
	void ConvertExrToNormalMapDDS(const HoudiniExrFileInfo& exrInfo, const std::filesystem::path& outPath);
	
	// 执行全部转换
	void ExecuteConvertAll();

	// 合成2DArray
	void ComposeHeightMap2DArray();
	void ComposeMinMaxZ2DArray();
	void ComposeSplatMap2DArray();
	void ComposeNormalMap2DArray();

	// 根据排序设置获取排序后的slice索引
	std::vector<int> GetSortedSliceIndices() const;

	// 解析文件名 "X_Y.exr" 获取坐标
	bool ParseTileCoord(const std::string& fileName, int& outX, int& outY);

private:
	bool m_bVisible = false;

	// === 第一列: Houdini EXR ===
	char m_houdiniBasePath[512] = "D:\\Houdini\\NixTerra0\\tex";
	std::vector<HoudiniExrFileInfo> m_heightExrFiles;
	std::vector<HoudiniExrFileInfo> m_splatExrFiles;
	std::vector<HoudiniExrFileInfo> m_normalExrFiles;
	int m_selectedHeightExrIndex = 0;
	int m_selectedSplatExrIndex = 0;
	int m_selectedNormalExrIndex = 0;

	// === 第二列: 转录DDS ===
	char m_nixOutputPath[512] = "D:\\NixAssets\\Terrain";
	std::vector<NixTerrainDdsInfo> m_nixDdsFiles;
	int m_selectedDdsIndex = 0;
	bool m_bConvertHeightMap = true;
	int m_heightMapRange[2] = { 0, 2048 };	// 高度图映射范围 [min, max]
	bool m_bConvertSplatMap = true;
	bool m_bConvertNormalMap = true;

	// === 第三列: 烘焙子区域纹理 ===
	char m_subTileOutputPath[512] = "D:\\NixAssets\\Terrain";	// 子区域纹理输出根路径
	bool m_bBakeHeightMapSubTiles = true;	// 烘焙HeightMap子区域
	bool m_bBakeSplatMapSubTiles = true;	// 烘焙SplatMap子区域
	bool m_bBakeNormalMapSubTiles = true;	// 烘焙NormalMap子区域
	bool m_bForceGenerateSubTiles = false;	// 强制生成（忽略已存在的文件）

	// === 第四列: 合成2DArray ===
	char m_heightArrayPath[512] = "D:\\NixAssets\\Terrain\\hMap2DArray.dds";
	char m_minMaxZPath[512] = "D:\\NixAssets\\Terrain\\mmz.dds";
	char m_splatArrayPath[512] = "D:\\NixAssets\\Terrain\\splatMap2DArray.dds";
	char m_normalArrayPath[512] = "D:\\NixAssets\\Terrain\\nMap2DArray.dds";
	bool m_bComposeHeightArray = true;
	bool m_bComposeMinMaxZ = true;
	bool m_bComposeSplatArray = true;
	bool m_bComposeNormalArray = true;
	bool m_bRowAscending = true;	// 行正序
	bool m_bColAscending = true;	// 列正序
	bool m_bColumnFirst = true;		// 先遍历列（不勾选则行优先）
	int m_terrainCountX = 0;		// 地形X方向数量(自动计算)
	int m_terrainCountY = 0;		// 地形Y方向数量(自动计算)
};
