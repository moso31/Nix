#pragma once
#include "Header.h"
#include <filesystem>

class NXGUITexture
{
public:
	NXGUITexture();
	~NXGUITexture() {}

	void Render();
	void Release();

	// set preview image.
	void SetImage(const std::filesystem::path& path);

	// 保存TextureNXInfo信息
	void SaveTextureNXInfo();

	// 重新加载纹理
	void ReloadTexture();

private:
	std::filesystem::path m_strImgPath;
	NXTexture2D* m_pTexImage;

	// 用于显示当前选中纹理的信息
	TextureNXInfo* m_pTexNXInfo;
};