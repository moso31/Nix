#pragma once
#include <functional>
#include <filesystem>
#include "NXTerrainCommon.h"

struct TerrainNodePath
{
	NXTerrainNodeId nodeId;
	std::filesystem::path path;
};

class NXTextureMaker
{
public:
	NXTextureMaker() {}
	virtual ~NXTextureMaker() {}

	// 生成地形总纹理，读取vector下的所有path并打包成一个数组
	static void GenerateTerrainHeightMap2DArray(const std::vector<TerrainNodePath>& rawPaths, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);

	// 生成地形最小最大Z值纹理，读取vector下的所有path并打包成一个数组
	static void GenerateTerrainMinMaxZMap2DArray(const std::vector<TerrainNodePath>& rawPaths, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);
};

