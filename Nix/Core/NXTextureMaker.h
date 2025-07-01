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

	// ���ɵ�����������ȡvector�µ�����path�������һ������
	static void GenerateTerrainHeightMap2DArray(const std::vector<TerrainNodePath>& rawPaths, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);

	// ���ɵ�����С���Zֵ������ȡvector�µ�����path�������һ������
	static void GenerateTerrainMinMaxZMap2DArray(const std::vector<TerrainNodePath>& rawPaths, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);
};

