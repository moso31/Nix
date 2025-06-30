#pragma once
#include <filesystem>
#include "Ntr.h"
#include "NXTextureDefinitions.h"

struct NXTerrainNormalMapBakeData
{
	float worldSize[2] = { 2048.0f, 2048.0f };
	float heightRange[2] = { 0.0f, 1000.0f };
};

class NXTexture;
class NXGUITexture
{
public:
	NXGUITexture();
	virtual ~NXGUITexture() {}

	void Render();
	void Release();

	// set preview image.
	void SetImage(const std::filesystem::path& path);

private:
	void Render_Texture();
	void Render_RawTexture();
	void Render_TexturePreview();
	void Render_Preview2D();

	void Render_BakePopup();
	void SaveNormalMap();

private:
	std::filesystem::path m_path;
	Ntr<NXTexture> m_pTexImage;
	NXTextureSerializationData m_texData;
	int m_guiImgRawWH[2] = { 1, 1 };

	// 如果是cube或者2DArray，允许预览单个slice
	int m_preview2DArraySliceIndex;

	// 由于目前基于这个GUI烘焙地形法线图
	// 所以需要提供一组参数明确地形的基本数据，比如大小和高度范围
	// todo：这玩意应该还是做一个terrainData专门存储比较好
	// todo：地形数据的设计思想还没确定；材质数据是和terrainData硬绑定，还是直接设计成材质参数？
	NXTerrainNormalMapBakeData m_terrainNormalMapBakeData;
};
