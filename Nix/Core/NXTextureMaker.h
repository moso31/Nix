#pragma once
#include <functional>
#include <filesystem>

class NXTextureMaker
{
public:
	NXTextureMaker() {}
	virtual ~NXTextureMaker() {}

	// ���ɵ�����������ȡvector�µ�����path�������һ������
	static void GenerateTerrainHeightMap2DArray(const std::vector<std::filesystem::path>& rawPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);

	// ���ɵ�����С���Zֵ������ȡvector�µ�����path�������һ������
	static void GenerateTerrainMinMaxZMap2DArray(const std::vector<std::filesystem::path>& rawPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount = nullptr);
};

