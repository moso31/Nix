#pragma once
#include "Header.h"

class NXGUITexture
{
public:
	NXGUITexture();
	~NXGUITexture() {}

	void Render();

private:
	// get preview image.
	NXTexture2D* GetImage(const std::string& strTexPath);

private:
	NXTexture2D* m_pImage;
};