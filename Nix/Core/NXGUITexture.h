#pragma once
#include "Header.h"
#include <filesystem>

struct NXTextureInfoData;
class NXGUITexture
{
public:
	NXGUITexture();
	~NXGUITexture() {}

	void Render();
	void Release();

	// set preview image.
	void SetImage(const std::filesystem::path& path, const NXTextureInfoData& texInfoData);

private:
	std::filesystem::path m_strImgPath;
	NXTexture2D* m_pTexImage;

	bool m_bGenerateMipMap;
	bool m_bInvertNormalY;
	int m_nTexType;
};