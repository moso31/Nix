#pragma once
#include "BaseDefs/DX12.h"
#include <functional>
#include <filesystem>
#include "NXTerrainCommon.h"

struct PerTerrainBakeData
{
	NXTerrainNodeId nodeId; // 地形节点ID
	std::filesystem::path pathHeightMap; // 高度图路径
	std::filesystem::path pathSplatMap; // SplatMap路径
};

struct TerrainTexLODBakeConfig
{
	std::vector<PerTerrainBakeData> bakeTerrains; // 需要烘焙的地形列表
	bool bForceGenerate = false; // 是否强制生成（忽略已存在的文件）
	bool bGenerateHeightMap = true; // 是否生成HeightMap
	bool bGenerateSplatMap = true; // 是否生成SplatMap
};

class NXTextureMaker
{
	static constexpr uint32_t kBaseSize = 2049;
	static constexpr DXGI_FORMAT kHeightMapFormat = DXGI_FORMAT_R16_UNORM;
	static constexpr DXGI_FORMAT kSplatMapFormat = DXGI_FORMAT_R16_UNORM;

public:
	NXTextureMaker() {}
	virtual ~NXTextureMaker() {}

	// 读地形原始R16 raw高度图数据
	static void ReadTerrainRawR16(const std::filesystem::path& path, std::vector<uint16_t>& out);

	// 读地形原始R8 dds splatmap 材质ID图数据
	static void ReadTerrainDDSR8Unorm(const std::filesystem::path& path, std::vector<uint8_t>& out);

	// 确保目录存在，若不存在则创建
	static void EnsureDir(const std::filesystem::path& dir);

	// 生成地形总纹理，读取vector下的所有path并打包成一个数组
	// nodeCountX/Y：地形节点的数量，用这个判断具体生成到2DArray的哪个sliceIndex
	// width/height：单个slice纹理大小
	static void GenerateTerrainHeightMap2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);

	// 生成地形最小最大Z值纹理，读取vector下的所有path并打包成一个数组
	// nodeCountX/Y：地形节点的数量，用这个判断具体生成到2DArray的哪个sliceIndex
	// width/height：单个slice纹理大小
	static void GenerateTerrainMinMaxZMap2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);

	// 生成地形法线
	// nodeCountX/Y：地形节点的数量，用这个判断具体生成到2DArray的哪个sliceIndex
	// width/height：单个slice纹理大小
	// zRange：以什么高度范围来计算法线
	static void GenerateTerrainNormal2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const Vector2& zRange = Vector2(0, 2048), const std::filesystem::path& outDDSPath = "", std::function<void()> onProgressCount = nullptr);

	// 生成地形流式加载LOD纹理集
	static void GenerateTerrainStreamingLODMaps(const TerrainTexLODBakeConfig& bakeConfig);

private:
	// 生成地形流式加载LOD纹理集-子函数
	static void GenerateTerrainStreamingLODMaps_HeightMap(const TerrainTexLODBakeConfig& bakeConfig);
	static void GenerateTerrainStreamingLODMaps_SplatMap(const TerrainTexLODBakeConfig& bakeConfig);

	// 保存地形Tile DDS纹理
	// src：源数据指针（整个高度图数据）
	// srcW/srcH：源数据宽高
	// startX/startY：从源数据的哪个位置开始拷贝 
	// tileSize：拷贝多大尺寸
	static void SaveTerrainTileHeightMap(const std::filesystem::path& outPath, const uint16_t* src, uint32_t srcW, uint32_t srcH, uint32_t startX, uint32_t startY, uint32_t tileSize);
	static void SaveTerrainTileSplatMap(const std::filesystem::path& outPath, const uint8_t* src, uint32_t srcW, uint32_t srcH, uint32_t startX, uint32_t startY, uint32_t tileSize);
};

