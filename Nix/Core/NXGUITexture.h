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

private:
	std::filesystem::path m_strImgPath;
	NXTexture2D* m_pTexImage;

	// ������ʾ��ǰѡ���������Ϣ
	TextureNXInfo* m_pTexInfo;
};