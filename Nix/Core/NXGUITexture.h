#pragma once
#include "Header.h"

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
	NXTexture* m_pTexImage;
};