#pragma once
#include "filesystem"

class NXTextureMaker
{
public:
	NXTextureMaker() {}
	virtual ~NXTextureMaker() {}

	// 生成地形总纹理，读取vector下的所有path并打包成一个数组
	static void GenerateTerrainHeightMap2DArray(const std::vector<std::filesystem::path>& rawPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath);

	// 生成地形最小最大Z值纹理，读取vector下的所有path并打包成一个数组
	static void GenerateTerrainMinMaxZMap2DArray(const std::vector<std::filesystem::path>& rawPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath);
};

